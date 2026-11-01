#include "themes.h"

QVector<LineTheme> LineTheme::all()
{
    QVector<LineTheme> out;

    { LineTheme t;
      t.id="classic"; t.name=QStringLiteral("经典 · 银灰");
      t.description=QStringLiteral("克制耐看的金属灰");
      t.ropeColor=QColor("#8A96A7"); t.ropeHighlight=QColor("#C8CFD8");
      t.ropeWidth=3.6; t.clipColors={QColor("#8A96A7"),QColor("#6C7688"),QColor("#A8B1BF")};
      out.append(t); }

    { LineTheme t;
      t.id="linen"; t.name=QStringLiteral("棉麻 · 木夹");
      t.description=QStringLiteral("木夹与麻绳的温和质感");
      t.ropeColor=QColor("#BAA58A"); t.ropeHighlight=QColor("#EBDFC9");
      t.ropeWidth=4.0; t.clipColors={QColor("#BAA58A"),QColor("#8F7C5F"),QColor("#D6C4A6")};
      out.append(t); }

    { LineTheme t;
      t.id="porcelain"; t.name=QStringLiteral("奶白 · 轻盈");
      t.description=QStringLiteral("干净通透的奶白");
      t.ropeColor=QColor("#AEB1B0"); t.ropeHighlight=QColor("#F3F0E8");
      t.ropeWidth=3.4; t.clipColors={QColor("#AEB1B0"),QColor("#C9C6BE"),QColor("#E3DFD5")};
      out.append(t); }

    { LineTheme t;
      t.id="color"; t.name=QStringLiteral("柔彩 · 日常");
      t.description=QStringLiteral("柔和的日常彩色");
      t.ropeColor=QColor("#A8ACAA"); t.ropeHighlight=QColor("#DADCD8");
      t.ropeWidth=3.4; t.clipColors={QColor("#F4A9A9"),QColor("#A9C0E8"),QColor("#B8D8B0"),QColor("#F2D08F")};
      out.append(t); }

    { LineTheme t;
      t.id="graphite"; t.name=QStringLiteral("石墨 · 金属");
      t.description=QStringLiteral("沉稳的金属石墨");
      t.ropeColor=QColor("#565D64"); t.ropeHighlight=QColor("#949BA3");
      t.ropeWidth=3.4; t.clipColors={QColor("#565D64"),QColor("#3E444A"),QColor("#7A828A")};
      out.append(t); }

    return out;
}

LineTheme LineTheme::byId(const QString &id)
{
    const auto list = all();
    for (const LineTheme &t : list)
        if (t.id == id) return t;
    return list.isEmpty() ? LineTheme() : list.first();
}
