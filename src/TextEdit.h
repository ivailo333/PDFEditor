#pragma once

#include <QColor>
#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>

struct TextEdit {
    int page = 0;
    QRectF pageRect;
    QSizeF pageSizePoints;
    QString text;
    QFont font = QFont(QStringLiteral("Arial"), 12);
    QColor color = Qt::black;
};
