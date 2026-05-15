#include "PdfDocumentView.h"

#include "QpdfBackend.h"

#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocumentRenderOptions>
#include <QPdfWriter>
#include <QTextEdit>

#include <algorithm>
#include <cmath>

namespace {
constexpr double BaseDpi = 144.0;
constexpr double PointsPerInch = 72.0;
constexpr double MinZoom = 0.35;
constexpr double MaxZoom = 4.0;
const QSizeF A4PagePoints(595.0, 842.0);
const QSizeF DefaultTextBoxSize(220.0, 72.0);

QSize renderSizeForPage(const QSizeF &pageSizePoints, double zoom)
{
    const double scale = (BaseDpi / PointsPerInch) * zoom;
    return QSize(
        std::max(1, static_cast<int>(std::round(pageSizePoints.width() * scale))),
        std::max(1, static_cast<int>(std::round(pageSizePoints.height() * scale))));
}
}

PdfDocumentView::PdfDocumentView(QWidget *parent)
    : QWidget(parent),
      m_document(std::make_unique<QPdfDocument>(this))
{
    setAutoFillBackground(true);
    setMouseTracking(true);
    setBackgroundRole(QPalette::Dark);
    setMinimumSize(720, 520);
    setFocusPolicy(Qt::StrongFocus);
}

bool PdfDocumentView::loadPdf(const QString &filePath, QString *errorMessage)
{
    auto document = std::make_unique<QPdfDocument>(this);
    const QPdfDocument::Error error = document->load(filePath);
    if (error != QPdfDocument::Error::None) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The PDF file cannot be opened.");
        }
        return false;
    }

    m_document = std::move(document);
    m_sourcePath = filePath;
    m_isCreatedDocument = false;
    m_createdPageCount = 0;
    m_currentPage = 0;
    m_zoom = 1.0;
    m_edits.clear();
    m_selectedEdit = -1;
    commitEditor();
    renderCurrentPage();
    return true;
}

void PdfDocumentView::createNewPdf()
{
    m_sourcePath.clear();
    m_isCreatedDocument = true;
    m_createdPageCount = 1;
    m_currentPage = 0;
    m_zoom = 1.0;
    m_edits.clear();
    m_selectedEdit = -1;
    commitEditor();
    m_pageSizePoints = A4PagePoints;
    m_renderedPage = QImage(renderSizeForPage(m_pageSizePoints, m_zoom), QImage::Format_ARGB32_Premultiplied);
    m_renderedPage.fill(Qt::white);
    updateCanvasSize();
    update();
}

void PdfDocumentView::addBlankPage()
{
    if (!m_isCreatedDocument) {
        return;
    }

    ++m_createdPageCount;
    setCurrentPage(m_createdPageCount - 1);
}

bool PdfDocumentView::savePdf(const QString &filePath, QString *errorMessage)
{
    commitEditor();

    if (!hasDocument()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No PDF document is loaded.");
        }
        return false;
    }

    if (m_isCreatedDocument) {
        return saveCreatedPdf(filePath, errorMessage);
    }

    const QFileInfo sourceInfo(m_sourcePath);
    const QFileInfo targetInfo(filePath);
    if (targetInfo.exists() && sourceInfo.canonicalFilePath() == targetInfo.canonicalFilePath()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Save the edited document as a new PDF file.");
        }
        return false;
    }

    return QpdfBackend::saveWithTextEdits(m_sourcePath, filePath, m_edits, errorMessage);
}

bool PdfDocumentView::hasDocument() const
{
    if (m_isCreatedDocument) {
        return m_createdPageCount > 0;
    }

    return m_document && m_document->status() == QPdfDocument::Status::Ready && pageCount() > 0;
}

bool PdfDocumentView::isCreatedDocument() const
{
    return m_isCreatedDocument;
}

int PdfDocumentView::currentPage() const
{
    return m_currentPage;
}

int PdfDocumentView::pageCount() const
{
    if (m_isCreatedDocument) {
        return m_createdPageCount;
    }

    return m_document ? m_document->pageCount() : 0;
}

double PdfDocumentView::zoomFactor() const
{
    return m_zoom;
}

void PdfDocumentView::nextPage()
{
    setCurrentPage(m_currentPage + 1);
}

void PdfDocumentView::previousPage()
{
    setCurrentPage(m_currentPage - 1);
}

void PdfDocumentView::zoomIn()
{
    m_zoom = std::min(MaxZoom, m_zoom * 1.2);
    renderCurrentPage();
}

void PdfDocumentView::zoomOut()
{
    m_zoom = std::max(MinZoom, m_zoom / 1.2);
    renderCurrentPage();
}

void PdfDocumentView::resetZoom()
{
    m_zoom = 1.0;
    renderCurrentPage();
}

void PdfDocumentView::setEditMode(bool enabled)
{
    m_editMode = enabled;
    if (!enabled) {
        commitEditor();
        m_selectedEdit = -1;
        m_draggingEdit = false;
    }
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
    update();
}

void PdfDocumentView::setTextForNewEdits(const QString &text)
{
    const QString trimmed = text.trimmed();
    m_newEditText = trimmed.isEmpty() ? QStringLiteral("Note") : trimmed;
}

void PdfDocumentView::setTextFontFamily(const QString &family)
{
    if (family.trimmed().isEmpty()) {
        return;
    }

    m_currentTextFont.setFamily(family);
    if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size())) {
        m_edits[static_cast<size_t>(m_selectedEdit)].font.setFamily(family);
    }
    if (m_textEditor) {
        m_textEditor->setFont(m_currentTextFont);
    }
    update();
}

void PdfDocumentView::setTextPointSize(int pointSize)
{
    const int clampedSize = std::clamp(pointSize, 6, 96);
    m_currentTextFont.setPointSize(clampedSize);
    if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size())) {
        m_edits[static_cast<size_t>(m_selectedEdit)].font.setPointSize(clampedSize);
    }
    if (m_textEditor) {
        m_textEditor->setFont(m_currentTextFont);
    }
    update();
}

void PdfDocumentView::setTextBold(bool enabled)
{
    m_currentTextFont.setBold(enabled);
    if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size())) {
        m_edits[static_cast<size_t>(m_selectedEdit)].font.setBold(enabled);
    }
    if (m_textEditor) {
        m_textEditor->setFont(m_currentTextFont);
    }
    update();
}

void PdfDocumentView::setTextItalic(bool enabled)
{
    m_currentTextFont.setItalic(enabled);
    if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size())) {
        m_edits[static_cast<size_t>(m_selectedEdit)].font.setItalic(enabled);
    }
    if (m_textEditor) {
        m_textEditor->setFont(m_currentTextFont);
    }
    update();
}

void PdfDocumentView::setTextUnderline(bool enabled)
{
    m_currentTextFont.setUnderline(enabled);
    if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size())) {
        m_edits[static_cast<size_t>(m_selectedEdit)].font.setUnderline(enabled);
    }
    if (m_textEditor) {
        m_textEditor->setFont(m_currentTextFont);
    }
    update();
}

void PdfDocumentView::clearCurrentPageEdits()
{
    m_edits.erase(
        std::remove_if(m_edits.begin(), m_edits.end(), [this](const TextEdit &edit) {
            return edit.page == m_currentPage;
        }),
        m_edits.end());
    m_selectedEdit = -1;
    commitEditor();
    update();
}

void PdfDocumentView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(45, 48, 53));

    if (!hasDocument()) {
        painter.setPen(QColor(235, 238, 242));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Open a PDF file to start."));
        return;
    }

    const QRectF targetRect = pageRect();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.fillRect(targetRect.translated(8, 10), QColor(20, 22, 26, 120));
    painter.fillRect(targetRect, Qt::white);
    painter.setPen(QColor(210, 210, 210));
    painter.drawRect(targetRect);

    if (!m_renderedPage.isNull()) {
        painter.drawImage(targetRect, m_renderedPage);
    } else if (m_isCreatedDocument) {
        paintBlankPage(painter, targetRect);
    }

    const double scale = targetRect.width() / m_pageSizePoints.width();
    for (size_t i = 0; i < m_edits.size(); ++i) {
        const TextEdit &edit = m_edits[i];
        if (edit.page != m_currentPage || static_cast<int>(i) == m_selectedEdit) {
            continue;
        }

        QFont font = edit.font;
        font.setPointSizeF(edit.font.pointSizeF() * scale);
        painter.setFont(font);
        painter.setPen(edit.color);
        painter.drawText(pageToWidgetRect(edit.pageRect).adjusted(4, 4, -4, -4),
                         Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                         edit.text);
    }

    if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size()) && !m_textEditor) {
        const TextEdit &edit = m_edits[static_cast<size_t>(m_selectedEdit)];
        if (edit.page == m_currentPage) {
            const QRectF selection = pageToWidgetRect(edit.pageRect);
            painter.setPen(QPen(QColor(25, 118, 210), 1.5, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(selection);
        }
    }
}

bool PdfDocumentView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_textEditor && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) &&
            (keyEvent->modifiers() & Qt::ControlModifier)) {
            commitEditor();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape) {
            commitEditor();
            return true;
        }
    }

    if (object == m_textEditor && event->type() == QEvent::FocusOut) {
        commitEditor();
        return false;
    }

    return QWidget::eventFilter(object, event);
}

void PdfDocumentView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete && m_selectedEdit >= 0 && !m_textEditor) {
        m_edits.erase(m_edits.begin() + m_selectedEdit);
        m_selectedEdit = -1;
        update();
        return;
    }

    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        (event->modifiers() & Qt::ControlModifier)) {
        commitEditor();
        return;
    }

    if (event->key() == Qt::Key_F2 && m_selectedEdit >= 0) {
        beginEditing(m_selectedEdit);
        return;
    }

    QWidget::keyPressEvent(event);
}

void PdfDocumentView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_editMode || !hasDocument()) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const int editIndex = editAt(event->position());
    if (editIndex >= 0) {
        beginEditing(editIndex);
    }
}

void PdfDocumentView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_draggingEdit || m_selectedEdit < 0 || m_selectedEdit >= static_cast<int>(m_edits.size()) || m_textEditor) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF currentPagePoint = widgetToPagePoint(event->position());
    const QPointF delta = currentPagePoint - m_dragStartPagePoint;
    QRectF movedRect = m_dragStartPageRect.translated(delta);
    movedRect.moveLeft(std::clamp(movedRect.left(), 0.0, std::max(0.0, m_pageSizePoints.width() - movedRect.width())));
    movedRect.moveTop(std::clamp(movedRect.top(), 0.0, std::max(0.0, m_pageSizePoints.height() - movedRect.height())));
    m_edits[static_cast<size_t>(m_selectedEdit)].pageRect = movedRect;
    update();
}

void PdfDocumentView::mousePressEvent(QMouseEvent *event)
{
    if (!m_editMode || !hasDocument() || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    setFocus();
    commitEditor();

    if (!pageRect().contains(event->position())) {
        m_selectedEdit = -1;
        update();
        return;
    }

    const int existingEdit = editAt(event->position());
    if (existingEdit >= 0) {
        m_selectedEdit = existingEdit;
        m_draggingEdit = true;
        m_dragStartPagePoint = widgetToPagePoint(event->position());
        m_dragStartPageRect = m_edits[static_cast<size_t>(existingEdit)].pageRect;
        update();
        return;
    }

    const QPointF pagePoint = widgetToPagePoint(event->position());
    TextEdit edit;
    edit.page = m_currentPage;
    edit.pageRect = defaultTextBoxAt(pagePoint);
    edit.pageSizePoints = m_pageSizePoints;
    edit.text = m_newEditText;
    edit.font = m_currentTextFont;
    edit.color = Qt::black;
    m_edits.push_back(edit);
    m_selectedEdit = static_cast<int>(m_edits.size()) - 1;
    beginEditing(m_selectedEdit);
    update();
}

void PdfDocumentView::mouseReleaseEvent(QMouseEvent *event)
{
    m_draggingEdit = false;
    QWidget::mouseReleaseEvent(event);
}

void PdfDocumentView::resizeEvent(QResizeEvent *)
{
    updateCanvasSize();
    updateEditorGeometry();
}

void PdfDocumentView::setCurrentPage(int page)
{
    if (!hasDocument()) {
        return;
    }

    const int clampedPage = std::clamp(page, 0, pageCount() - 1);
    if (clampedPage == m_currentPage && !m_renderedPage.isNull()) {
        return;
    }

    commitEditor();
    m_currentPage = clampedPage;
    m_selectedEdit = -1;
    renderCurrentPage();
}

void PdfDocumentView::renderCurrentPage()
{
    if (!hasDocument()) {
        m_renderedPage = QImage();
        update();
        return;
    }

    if (m_isCreatedDocument) {
        m_pageSizePoints = A4PagePoints;
        m_renderedPage = QImage(renderSizeForPage(m_pageSizePoints, m_zoom), QImage::Format_ARGB32_Premultiplied);
        m_renderedPage.fill(Qt::white);
    } else {
        m_pageSizePoints = m_document->pagePointSize(m_currentPage);
        m_renderedPage = m_document->render(m_currentPage,
                                            renderSizeForPage(m_pageSizePoints, m_zoom),
                                            QPdfDocumentRenderOptions());
    }
    updateCanvasSize();
    updateEditorGeometry();
    update();
}

void PdfDocumentView::updateCanvasSize()
{
    if (m_renderedPage.isNull()) {
        setMinimumSize(720, 520);
        return;
    }

    setMinimumSize(std::min(m_renderedPage.width() + 48, 1800),
                   std::min(m_renderedPage.height() + 48, 2000));
}

QRectF PdfDocumentView::pageRect() const
{
    if (m_renderedPage.isNull()) {
        return QRectF();
    }

    const QSizeF imageSize = m_renderedPage.size();
    const QSizeF available(std::max(1, width() - 48), std::max(1, height() - 48));
    const double fitScale = std::min(available.width() / imageSize.width(),
                                     available.height() / imageSize.height());
    const QSizeF displaySize = imageSize * std::max(0.01, fitScale);
    return QRectF(QPointF((width() - displaySize.width()) / 2.0,
                          (height() - displaySize.height()) / 2.0),
                  displaySize);
}

QPointF PdfDocumentView::widgetToPagePoint(const QPointF &point) const
{
    const QRectF rect = pageRect();
    const double scale = rect.width() / m_pageSizePoints.width();
    return QPointF((point.x() - rect.left()) / scale,
                   (point.y() - rect.top()) / scale);
}

QRectF PdfDocumentView::pageToWidgetRect(const QRectF &rect) const
{
    const QRectF pageBounds = pageRect();
    const double scale = pageBounds.width() / m_pageSizePoints.width();
    return QRectF(pageBounds.left() + rect.left() * scale,
                  pageBounds.top() + rect.top() * scale,
                  rect.width() * scale,
                  rect.height() * scale);
}

QRectF PdfDocumentView::defaultTextBoxAt(const QPointF &pagePoint) const
{
    QRectF rect(pagePoint, DefaultTextBoxSize);
    if (rect.right() > m_pageSizePoints.width()) {
        rect.moveRight(m_pageSizePoints.width());
    }
    if (rect.bottom() > m_pageSizePoints.height()) {
        rect.moveBottom(m_pageSizePoints.height());
    }
    return rect;
}

int PdfDocumentView::editAt(const QPointF &widgetPoint) const
{
    for (int i = static_cast<int>(m_edits.size()) - 1; i >= 0; --i) {
        const TextEdit &edit = m_edits[static_cast<size_t>(i)];
        if (edit.page == m_currentPage && pageToWidgetRect(edit.pageRect).contains(widgetPoint)) {
            return i;
        }
    }

    return -1;
}

void PdfDocumentView::beginEditing(int editIndex)
{
    if (editIndex < 0 || editIndex >= static_cast<int>(m_edits.size())) {
        return;
    }

    commitEditor();
    m_selectedEdit = editIndex;

    m_textEditor = new QTextEdit(this);
    m_textEditor->setAcceptRichText(false);
    m_textEditor->setFrameShape(QFrame::Box);
    m_textEditor->setFont(m_edits[static_cast<size_t>(editIndex)].font);
    m_textEditor->setTextColor(m_edits[static_cast<size_t>(editIndex)].color);
    m_textEditor->setPlainText(m_edits[static_cast<size_t>(editIndex)].text);
    m_textEditor->installEventFilter(this);
    connect(m_textEditor, &QTextEdit::textChanged, this, [this] {
        if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size()) && m_textEditor) {
            m_edits[static_cast<size_t>(m_selectedEdit)].text = m_textEditor->toPlainText();
            update();
        }
    });
    updateEditorGeometry();
    m_textEditor->show();
    m_textEditor->setFocus();
    m_textEditor->selectAll();
    update();
}

void PdfDocumentView::commitEditor()
{
    if (!m_textEditor) {
        return;
    }

    if (m_selectedEdit >= 0 && m_selectedEdit < static_cast<int>(m_edits.size())) {
        m_edits[static_cast<size_t>(m_selectedEdit)].text = m_textEditor->toPlainText();
    }

    m_textEditor->removeEventFilter(this);
    m_textEditor->deleteLater();
    m_textEditor = nullptr;
    update();
}

void PdfDocumentView::updateEditorGeometry()
{
    if (!m_textEditor || m_selectedEdit < 0 || m_selectedEdit >= static_cast<int>(m_edits.size())) {
        return;
    }

    m_textEditor->setGeometry(pageToWidgetRect(m_edits[static_cast<size_t>(m_selectedEdit)].pageRect).toAlignedRect());
}

std::vector<TextEdit> PdfDocumentView::editsForPage(int page) const
{
    std::vector<TextEdit> result;
    std::copy_if(m_edits.begin(), m_edits.end(), std::back_inserter(result), [page](const TextEdit &edit) {
        return edit.page == page;
    });
    return result;
}

void PdfDocumentView::paintBlankPage(QPainter &painter, const QRectF &targetRect) const
{
    painter.fillRect(targetRect, Qt::white);
}

bool PdfDocumentView::saveCreatedPdf(const QString &filePath, QString *errorMessage) const
{
    QPdfWriter writer(filePath);
    writer.setCreator(QStringLiteral("PDFEditor"));
    writer.setResolution(static_cast<int>(PointsPerInch));
    writer.setPageSize(QPageSize(A4PagePoints, QPageSize::Point));

    QPainter painter(&writer);
    if (!painter.isActive()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The PDF file cannot be created.");
        }
        return false;
    }

    for (int page = 0; page < m_createdPageCount; ++page) {
        if (page > 0) {
            writer.newPage();
        }

        painter.fillRect(QRectF(QPointF(0, 0), A4PagePoints), Qt::white);
        for (const TextEdit &edit : editsForPage(page)) {
            painter.setFont(edit.font);
            painter.setPen(edit.color);
            painter.drawText(edit.pageRect.adjusted(4, 4, -4, -4),
                             Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                             edit.text);
        }
    }

    painter.end();
    return true;
}
