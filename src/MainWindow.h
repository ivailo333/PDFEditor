#pragma once

#include <QMainWindow>

class QAction;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class PdfDocumentView;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void newPdf();
    void openPdf();
    void savePdfAs();
    void updateActions();

    PdfDocumentView *m_view = nullptr;
    QAction *m_addPageAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_previousAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_zoomInAction = nullptr;
    QAction *m_zoomOutAction = nullptr;
    QAction *m_resetZoomAction = nullptr;
    QAction *m_editModeAction = nullptr;
    QAction *m_boldAction = nullptr;
    QAction *m_italicAction = nullptr;
    QAction *m_underlineAction = nullptr;
    QAction *m_clearPageEditsAction = nullptr;
    QLabel *m_pageLabel = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QLineEdit *m_textEdit = nullptr;
    QFontComboBox *m_fontComboBox = nullptr;
    QSpinBox *m_fontSizeSpinBox = nullptr;
};
