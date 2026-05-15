#include "MainWindow.h"

#include "PdfDocumentView.h"

#include <QAction>
#include <QFileDialog>
#include <QFontComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_view = new PdfDocumentView(this);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(m_view);
    scrollArea->setWidgetResizable(true);
    setCentralWidget(scrollArea);

    auto *toolbar = addToolBar(QStringLiteral("PDF"));
    toolbar->setMovable(false);

    QAction *newAction = toolbar->addAction(QStringLiteral("New"));
    QAction *openAction = toolbar->addAction(QStringLiteral("Open"));
    m_saveAction = toolbar->addAction(QStringLiteral("Save As"));
    toolbar->addSeparator();

    m_previousAction = toolbar->addAction(QStringLiteral("Previous"));
    m_pageLabel = new QLabel(QStringLiteral("Page -/-"), this);
    m_pageLabel->setMinimumWidth(90);
    m_pageLabel->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(m_pageLabel);
    m_nextAction = toolbar->addAction(QStringLiteral("Next"));
    toolbar->addSeparator();

    m_zoomOutAction = toolbar->addAction(QStringLiteral("-"));
    m_zoomLabel = new QLabel(QStringLiteral("100%"), this);
    m_zoomLabel->setMinimumWidth(60);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(m_zoomLabel);
    m_zoomInAction = toolbar->addAction(QStringLiteral("+"));
    m_resetZoomAction = toolbar->addAction(QStringLiteral("100%"));
    toolbar->addSeparator();

    m_editModeAction = toolbar->addAction(QStringLiteral("Edit"));
    m_editModeAction->setCheckable(true);

    m_textEdit = new QLineEdit(this);
    m_textEdit->setText(QStringLiteral("Note"));
    m_textEdit->setPlaceholderText(QStringLiteral("Text to add"));
    m_textEdit->setMinimumWidth(220);
    toolbar->addWidget(m_textEdit);

    m_fontComboBox = new QFontComboBox(this);
    m_fontComboBox->setCurrentFont(QFont(QStringLiteral("Arial")));
    m_fontComboBox->setMinimumWidth(170);
    toolbar->addWidget(m_fontComboBox);

    m_fontSizeSpinBox = new QSpinBox(this);
    m_fontSizeSpinBox->setRange(6, 96);
    m_fontSizeSpinBox->setValue(12);
    m_fontSizeSpinBox->setSuffix(QStringLiteral(" pt"));
    m_fontSizeSpinBox->setMinimumWidth(76);
    toolbar->addWidget(m_fontSizeSpinBox);

    m_boldAction = toolbar->addAction(QStringLiteral("B"));
    m_boldAction->setCheckable(true);
    m_boldAction->setToolTip(QStringLiteral("Bold"));
    QFont boldButtonFont = m_boldAction->font();
    boldButtonFont.setBold(true);
    m_boldAction->setFont(boldButtonFont);

    m_italicAction = toolbar->addAction(QStringLiteral("I"));
    m_italicAction->setCheckable(true);
    m_italicAction->setToolTip(QStringLiteral("Italic"));
    QFont italicButtonFont = m_italicAction->font();
    italicButtonFont.setItalic(true);
    m_italicAction->setFont(italicButtonFont);

    m_underlineAction = toolbar->addAction(QStringLiteral("U"));
    m_underlineAction->setCheckable(true);
    m_underlineAction->setToolTip(QStringLiteral("Underline"));
    QFont underlineButtonFont = m_underlineAction->font();
    underlineButtonFont.setUnderline(true);
    m_underlineAction->setFont(underlineButtonFont);

    m_clearPageEditsAction = toolbar->addAction(QStringLiteral("Clear Page"));
    m_addPageAction = toolbar->addAction(QStringLiteral("Add Page"));

    connect(newAction, &QAction::triggered, this, [this] { newPdf(); });
    connect(openAction, &QAction::triggered, this, [this] { openPdf(); });
    connect(m_saveAction, &QAction::triggered, this, [this] { savePdfAs(); });
    connect(m_previousAction, &QAction::triggered, this, [this] {
        m_view->previousPage();
        updateActions();
    });
    connect(m_nextAction, &QAction::triggered, this, [this] {
        m_view->nextPage();
        updateActions();
    });
    connect(m_zoomInAction, &QAction::triggered, this, [this] {
        m_view->zoomIn();
        updateActions();
    });
    connect(m_zoomOutAction, &QAction::triggered, this, [this] {
        m_view->zoomOut();
        updateActions();
    });
    connect(m_resetZoomAction, &QAction::triggered, this, [this] {
        m_view->resetZoom();
        updateActions();
    });
    connect(m_editModeAction, &QAction::toggled, m_view, &PdfDocumentView::setEditMode);
    connect(m_textEdit, &QLineEdit::textChanged, m_view, &PdfDocumentView::setTextForNewEdits);
    connect(m_fontComboBox, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        m_view->setTextFontFamily(font.family());
    });
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_view, &PdfDocumentView::setTextPointSize);
    connect(m_boldAction, &QAction::toggled, m_view, &PdfDocumentView::setTextBold);
    connect(m_italicAction, &QAction::toggled, m_view, &PdfDocumentView::setTextItalic);
    connect(m_underlineAction, &QAction::toggled, m_view, &PdfDocumentView::setTextUnderline);
    connect(m_clearPageEditsAction, &QAction::triggered, m_view, &PdfDocumentView::clearCurrentPageEdits);
    connect(m_addPageAction, &QAction::triggered, this, [this] {
        m_view->addBlankPage();
        updateActions();
    });

    setWindowTitle(QStringLiteral("PDFEditor"));
    statusBar()->showMessage(QStringLiteral("Ready"));
    updateActions();
}

void MainWindow::newPdf()
{
    m_view->createNewPdf();
    setWindowTitle(QStringLiteral("PDFEditor - New document"));
    statusBar()->showMessage(QStringLiteral("New PDF document created"), 4000);
    updateActions();
}

void MainWindow::openPdf()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open PDF"),
        QString(),
        QStringLiteral("PDF files (*.pdf)"));

    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_view->loadPdf(filePath, &errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("Open failed"), errorMessage);
        return;
    }

    setWindowTitle(QStringLiteral("PDFEditor - %1").arg(filePath));
    updateActions();
}

void MainWindow::savePdfAs()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save PDF"),
        QStringLiteral("edited.pdf"),
        QStringLiteral("PDF files (*.pdf)"));

    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_view->savePdf(filePath, &errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), errorMessage);
        return;
    }

    statusBar()->showMessage(QStringLiteral("Saved: %1").arg(filePath), 6000);
}

void MainWindow::updateActions()
{
    const bool hasDocument = m_view->hasDocument();
    m_saveAction->setEnabled(hasDocument);
    m_previousAction->setEnabled(hasDocument && m_view->currentPage() > 0);
    m_nextAction->setEnabled(hasDocument && m_view->currentPage() < m_view->pageCount() - 1);
    m_zoomInAction->setEnabled(hasDocument);
    m_zoomOutAction->setEnabled(hasDocument);
    m_resetZoomAction->setEnabled(hasDocument);
    m_editModeAction->setEnabled(hasDocument);
    m_clearPageEditsAction->setEnabled(hasDocument);
    m_addPageAction->setEnabled(hasDocument && m_view->isCreatedDocument());
    m_textEdit->setEnabled(hasDocument);
    m_fontComboBox->setEnabled(hasDocument);
    m_fontSizeSpinBox->setEnabled(hasDocument);
    m_boldAction->setEnabled(hasDocument);
    m_italicAction->setEnabled(hasDocument);
    m_underlineAction->setEnabled(hasDocument);

    if (hasDocument) {
        m_pageLabel->setText(QStringLiteral("Page %1/%2").arg(m_view->currentPage() + 1).arg(m_view->pageCount()));
        m_zoomLabel->setText(QStringLiteral("%1%").arg(static_cast<int>(m_view->zoomFactor() * 100.0)));
    } else {
        m_pageLabel->setText(QStringLiteral("Page -/-"));
        m_zoomLabel->setText(QStringLiteral("100%"));
    }
}
