#include "QpdfBackend.h"

#include <QByteArray>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFWriter.hh>

#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
constexpr const char *AppearanceFontName = "/F1";

std::string pdfBaseFontFor(const QFont &font)
{
    if (font.bold() && font.italic()) {
        return "/Helvetica-BoldOblique";
    }
    if (font.bold()) {
        return "/Helvetica-Bold";
    }
    if (font.italic()) {
        return "/Helvetica-Oblique";
    }
    return "/Helvetica";
}

std::string pathForQpdf(const QString &path)
{
    return path.toUtf8().toStdString();
}

std::string escapePdfLiteralString(const QString &text)
{
    const QByteArray bytes = text.toUtf8();
    std::string escaped;
    escaped.reserve(static_cast<size_t>(bytes.size()));

    for (const unsigned char ch : bytes) {
        switch (ch) {
        case '(':
            escaped += "\\(";
            break;
        case ')':
            escaped += "\\)";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(static_cast<char>(ch));
            break;
        }
    }

    return escaped;
}

std::string makeAppearanceStream(const TextEdit &edit)
{
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(3);

    const double fontSize = edit.font.pointSizeF() > 0.0 ? edit.font.pointSizeF() : 12.0;
    const double textY = std::max(4.0, edit.pageRect.height() - fontSize - 4.0);

    stream << "q\n"
           << "BT\n"
           << AppearanceFontName << " " << fontSize << " Tf\n"
           << edit.color.redF() << " " << edit.color.greenF() << " " << edit.color.blueF() << " rg\n"
           << "1 0 0 1 4 " << textY << " Tm\n"
           << "(" << escapePdfLiteralString(edit.text) << ") Tj\n"
           << "ET\n"
           << "Q\n";

    if (edit.font.underline()) {
        const double underlineY = std::max(2.0, textY - 2.0);
        stream << "q\n"
               << edit.color.redF() << " " << edit.color.greenF() << " " << edit.color.blueF() << " RG\n"
               << "0.8 w\n"
               << "4 " << underlineY << " m\n"
               << std::max(4.0, edit.pageRect.width() - 4.0) << " " << underlineY << " l\n"
               << "S\n"
               << "Q\n";
    }

    return stream.str();
}

QPDFObjectHandle makeFreeTextAnnotation(QPDF &pdf, const TextEdit &edit)
{
    const double x1 = edit.pageRect.left();
    const double y2 = edit.pageSizePoints.height() - edit.pageRect.top();
    const double x2 = edit.pageRect.right();
    const double y1 = edit.pageSizePoints.height() - edit.pageRect.bottom();

    QPDFObjectHandle appearance = QPDFObjectHandle::newStream(&pdf, makeAppearanceStream(edit));
    std::ostringstream appearanceDictionaryText;
    appearanceDictionaryText.setf(std::ios::fixed);
    appearanceDictionaryText.precision(3);
    appearanceDictionaryText
        << "<< /Type /XObject /Subtype /Form /BBox [0 0 "
        << edit.pageRect.width() << " " << edit.pageRect.height() << "] "
        << "/Resources << /Font << /F1 << /Type /Font /Subtype /Type1 /BaseFont "
        << pdfBaseFontFor(edit.font)
        << " >> >> >> >>";
    appearance.replaceDict(QPDFObjectHandle::parse(appearanceDictionaryText.str()));

    std::ostringstream rect;
    rect.setf(std::ios::fixed);
    rect.precision(3);
    rect << "[" << x1 << " " << y1 << " " << x2 << " " << y2 << "]";

    QPDFObjectHandle appearanceDictionary = QPDFObjectHandle::newDictionary();
    appearanceDictionary.replaceKey("/N", appearance);

    QPDFObjectHandle dictionary = QPDFObjectHandle::newDictionary();
    dictionary.replaceKey("/Type", QPDFObjectHandle::newName("/Annot"));
    dictionary.replaceKey("/Subtype", QPDFObjectHandle::newName("/FreeText"));
    dictionary.replaceKey("/Rect", QPDFObjectHandle::parse(rect.str()));
    dictionary.replaceKey("/Contents", QPDFObjectHandle::newUnicodeString(edit.text.toUtf8().toStdString()));
    dictionary.replaceKey("/DA", QPDFObjectHandle::parse("(0 0 0 rg /Helv 12 Tf)"));
    dictionary.replaceKey("/F", QPDFObjectHandle::newInteger(4));
    dictionary.replaceKey("/AP", appearanceDictionary);

    return pdf.makeIndirectObject(dictionary);
}

void appendAnnotation(QPDFObjectHandle page, QPDFObjectHandle annotation)
{
    QPDFObjectHandle annotations = page.getKey("/Annots");

    if (annotations.isNull()) {
        QPDFObjectHandle array = QPDFObjectHandle::newArray();
        array.appendItem(annotation);
        page.replaceKey("/Annots", array);
        return;
    }

    if (annotations.isArray()) {
        annotations.appendItem(annotation);
        return;
    }

    QPDFObjectHandle array = QPDFObjectHandle::newArray();
    array.appendItem(annotations);
    array.appendItem(annotation);
    page.replaceKey("/Annots", array);
}
}

bool QpdfBackend::saveWithTextEdits(const QString &sourcePath,
                                    const QString &targetPath,
                                    const std::vector<TextEdit> &edits,
                                    QString *errorMessage)
{
    try {
        QPDF pdf;
        pdf.processFile(pathForQpdf(sourcePath).c_str());

        const std::vector<QPDFObjectHandle> pages = pdf.getAllPages();
        std::map<int, std::vector<TextEdit>> editsByPage;
        for (const TextEdit &edit : edits) {
            editsByPage[edit.page].push_back(edit);
        }

        for (const auto &[pageIndex, pageEdits] : editsByPage) {
            if (pageIndex < 0 || pageIndex >= static_cast<int>(pages.size())) {
                continue;
            }

            QPDFObjectHandle page = pages[static_cast<size_t>(pageIndex)];
            for (const TextEdit &edit : pageEdits) {
                appendAnnotation(page, makeFreeTextAnnotation(pdf, edit));
            }
        }

        QPDFWriter writer(pdf, pathForQpdf(targetPath).c_str());
        writer.setStaticID(true);
        writer.write();
        return true;
    } catch (const std::exception &exception) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("QPDF failed to save the edited PDF: %1")
                                .arg(QString::fromUtf8(exception.what()));
        }
        return false;
    }
}
