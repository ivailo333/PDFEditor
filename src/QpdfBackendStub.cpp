#include "QpdfBackend.h"

bool QpdfBackend::saveWithTextEdits(const QString &,
                                    const QString &,
                                    const std::vector<TextEdit> &,
                                    QString *errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("QPDF is not configured. Set QPDF_ROOT in Qt Creator and run qmake again.");
    }

    return false;
}
