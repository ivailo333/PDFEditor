#pragma once

#include "TextEdit.h"

#include <QImage>
#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QWidget>

#include <memory>
#include <vector>

#ifdef PDFEDITOR_HAS_QTPDF
#include <QPdfDocument>
#endif

class QPainter;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QTextEdit;

class PdfDocumentView : public QWidget {
public:
    explicit PdfDocumentView(QWidget *parent = nullptr);

    bool loadPdf(const QString &filePath, QString *errorMessage = nullptr);
    void createNewPdf();
    void addBlankPage();
    bool savePdf(const QString &filePath, QString *errorMessage = nullptr);

    bool hasDocument() const;
    bool isCreatedDocument() const;
    int currentPage() const;
    int pageCount() const;
    double zoomFactor() const;

    void nextPage();
    void previousPage();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setEditMode(bool enabled);
    void setTextForNewEdits(const QString &text);
    void setTextFontFamily(const QString &family);
    void setTextPointSize(int pointSize);
    void setTextBold(bool enabled);
    void setTextItalic(bool enabled);
    void setTextUnderline(bool enabled);
    void clearCurrentPageEdits();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setCurrentPage(int page);
    void renderCurrentPage();
    void updateCanvasSize();
    QRectF pageRect() const;
    QPointF widgetToPagePoint(const QPointF &point) const;
    QRectF pageToWidgetRect(const QRectF &rect) const;
    QRectF defaultTextBoxAt(const QPointF &pagePoint) const;
    int editAt(const QPointF &widgetPoint) const;
    void beginEditing(int editIndex);
    void commitEditor();
    void updateEditorGeometry();
    std::vector<TextEdit> editsForPage(int page) const;
    void paintBlankPage(QPainter &painter, const QRectF &targetRect) const;
    bool saveCreatedPdf(const QString &filePath, QString *errorMessage) const;

    QString m_sourcePath;
    bool m_isCreatedDocument = false;
    int m_currentPage = 0;
    int m_createdPageCount = 0;
    double m_zoom = 1.0;
    bool m_editMode = false;
    QString m_newEditText = QStringLiteral("Note");
    QFont m_currentTextFont = QFont(QStringLiteral("Arial"), 12);
    QImage m_renderedPage;
    QSizeF m_pageSizePoints;
    std::vector<TextEdit> m_edits;
    int m_selectedEdit = -1;
    bool m_draggingEdit = false;
    QPointF m_dragStartPagePoint;
    QRectF m_dragStartPageRect;
    QTextEdit *m_textEditor = nullptr;

#ifdef PDFEDITOR_HAS_QTPDF
    std::unique_ptr<QPdfDocument> m_document;
#endif
};
