#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PDFEditor"));

    MainWindow window;
    window.resize(1100, 800);
    window.show();

    return app.exec();
}
