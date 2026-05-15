#pragma once

#include "TextEdit.h"

#include <QString>

#include <vector>

class QpdfBackend {
public:
    static bool saveWithTextEdits(const QString &sourcePath,
                                  const QString &targetPath,
                                  const std::vector<TextEdit> &edits,
                                  QString *errorMessage = nullptr);
};
