QT += core gui widgets

TEMPLATE = app
TARGET = PDFEditor
CONFIG += c++17
CONFIG -= app_bundle

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp

HEADERS += \
    src/MainWindow.h \
    src/PdfDocumentView.h \
    src/QpdfBackend.h \
    src/TextEdit.h

qtHaveModule(pdf) {
    QT += pdf
    DEFINES += PDFEDITOR_HAS_QTPDF
    SOURCES += src/PdfDocumentViewQt.cpp
    message("Qt PDF module found. PDF viewing is enabled.")
} else {
    SOURCES += src/PdfDocumentViewStub.cpp
    warning("Qt PDF module was not found. The project will open, but PDF viewing is disabled.")
}

win32 {
    QPDF_ROOT = $$getenv(QPDF_ROOT)
    isEmpty(QPDF_ROOT): QPDF_ROOT = C:/vcpkg/installed/x64-mingw-dynamic

    exists($$QPDF_ROOT/include/qpdf/QPDF.hh) {
        DEFINES += PDFEDITOR_HAS_QPDF
        INCLUDEPATH += $$QPDF_ROOT/include
        LIBS += $$quote(-L$$QPDF_ROOT/lib) -lqpdf
        SOURCES += src/QpdfBackend.cpp
        message("QPDF found at $$QPDF_ROOT. PDF saving is enabled.")
    } else {
        SOURCES += src/QpdfBackendStub.cpp
        warning("QPDF was not found. Set QPDF_ROOT to enable real PDF saving.")
    }
}

unix {
    CONFIG += link_pkgconfig
    packagesExist(libqpdf) {
        DEFINES += PDFEDITOR_HAS_QPDF
        PKGCONFIG += libqpdf
        SOURCES += src/QpdfBackend.cpp
        message("QPDF found by pkg-config. PDF saving is enabled.")
    } else {
        SOURCES += src/QpdfBackendStub.cpp
        warning("QPDF was not found. Install libqpdf-dev to enable real PDF saving.")
    }
}

msvc {
    QMAKE_CXXFLAGS += /utf-8
}
