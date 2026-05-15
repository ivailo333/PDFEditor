#include "PdfDocumentView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

PdfDocumentView::PdfDocumentView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(720, 520);
}

bool PdfDocumentView::loadPdf(const QString &, QString *errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Qt PDF module is not installed for this Qt Kit. Install Qt PDF and run qmake again.");
    }

    return false;
}

void PdfDocumentView::createNewPdf() {}
void PdfDocumentView::addBlankPage() {}

bool PdfDocumentView::savePdf(const QString &, QString *errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("No PDF document is loaded.");
    }

    return false;
}

bool PdfDocumentView::hasDocument() const { return false; }
bool PdfDocumentView::isCreatedDocument() const { return false; }
int PdfDocumentView::currentPage() const { return 0; }
int PdfDocumentView::pageCount() const { return 0; }
double PdfDocumentView::zoomFactor() const { return 1.0; }
void PdfDocumentView::nextPage() {}
void PdfDocumentView::previousPage() {}
void PdfDocumentView::zoomIn() {}
void PdfDocumentView::zoomOut() {}
void PdfDocumentView::resetZoom() {}
void PdfDocumentView::setEditMode(bool) {}
void PdfDocumentView::setTextForNewEdits(const QString &) {}
void PdfDocumentView::setTextFontFamily(const QString &) {}
void PdfDocumentView::setTextPointSize(int) {}
void PdfDocumentView::setTextBold(bool) {}
void PdfDocumentView::setTextItalic(bool) {}
void PdfDocumentView::setTextUnderline(bool) {}
void PdfDocumentView::clearCurrentPageEdits() {}
void PdfDocumentView::mousePressEvent(QMouseEvent *event) { QWidget::mousePressEvent(event); }
bool PdfDocumentView::eventFilter(QObject *object, QEvent *event) { return QWidget::eventFilter(object, event); }
void PdfDocumentView::keyPressEvent(QKeyEvent *event) { QWidget::keyPressEvent(event); }
void PdfDocumentView::mouseDoubleClickEvent(QMouseEvent *event) { QWidget::mouseDoubleClickEvent(event); }
void PdfDocumentView::mouseMoveEvent(QMouseEvent *event) { QWidget::mouseMoveEvent(event); }
void PdfDocumentView::mouseReleaseEvent(QMouseEvent *event) { QWidget::mouseReleaseEvent(event); }
void PdfDocumentView::resizeEvent(QResizeEvent *) {}
void PdfDocumentView::setCurrentPage(int) {}
void PdfDocumentView::renderCurrentPage() {}
void PdfDocumentView::updateCanvasSize() {}
QRectF PdfDocumentView::pageRect() const { return QRectF(); }
QPointF PdfDocumentView::widgetToPagePoint(const QPointF &) const { return QPointF(); }
QRectF PdfDocumentView::pageToWidgetRect(const QRectF &) const { return QRectF(); }
QRectF PdfDocumentView::defaultTextBoxAt(const QPointF &) const { return QRectF(); }
int PdfDocumentView::editAt(const QPointF &) const { return -1; }
void PdfDocumentView::beginEditing(int) {}
void PdfDocumentView::commitEditor() {}
void PdfDocumentView::updateEditorGeometry() {}
std::vector<TextEdit> PdfDocumentView::editsForPage(int) const { return {}; }
void PdfDocumentView::paintBlankPage(QPainter &, const QRectF &) const {}
bool PdfDocumentView::saveCreatedPdf(const QString &, QString *) const { return false; }

void PdfDocumentView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(45, 48, 53));
    painter.setPen(QColor(235, 238, 242));
    painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Qt PDF is not installed for this Qt Kit."));
}
