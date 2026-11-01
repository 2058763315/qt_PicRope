#pragma once
// =====================================================================
//  ui/themes.h — 五套外观（对应原 PicRope 的 LineTheme.cs）
// =====================================================================

#include <QString>
#include <QColor>
#include <QVector>

struct LineTheme
{
    QString id;
    QString name;
    QString description;
    QColor  ropeColor;
    QColor  ropeHighlight;
    double  ropeWidth;
    QVector<QColor> clipColors;   // 夹子颜色（可多色循环）

    static QVector<LineTheme> all();
    static LineTheme byId(const QString &id);
};
