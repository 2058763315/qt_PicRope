#include "state.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QStandardPaths>
#include <QCoreApplication>

namespace StateJson {

QJsonObject textToJson(const TextItem &t)
{
    QJsonObject o;
    o["id"] = t.id;
    o["content"] = t.content;
    o["createdUtc"] = t.createdUtc.toString(Qt::ISODate);
    return o;
}

TextItem textFromJson(const QJsonObject &o)
{
    TextItem t;
    t.id = o["id"].toString();
    t.content = o["content"].toString();
    t.createdUtc = QDateTime::fromString(o["createdUtc"].toString(), Qt::ISODate);
    if (!t.createdUtc.isValid()) t.createdUtc = QDateTime::currentDateTimeUtc();
    return t;
}

QJsonObject toJson(const SavedState &s)
{
    QJsonObject o;
    o["CollectClipboard"] = s.collectClipboard;
    o["CollectTextClipboard"] = s.collectTextClipboard;
    o["AutoHang"] = s.autoHang;
    o["HoverPreview"] = s.hoverPreview;
    o["Theme"] = s.theme;
    o["DisplayDevice"] = s.displayDevice;
    o["LastUpdateNotification"] = s.lastUpdateNotification;
    o["CaptureModifiers"] = static_cast<int>(s.captureModifiers);
    o["CaptureKey"] = static_cast<int>(s.captureKey);

    QJsonArray imgs;
    for (const QString &p : s.images) imgs.append(p);
    o["Images"] = imgs;

    QJsonArray coll;
    for (const QString &p : s.collection) coll.append(p);
    o["Collection"] = coll;

    QJsonArray texts;
    for (const TextItem &t : s.texts) texts.append(textToJson(t));
    o["Texts"] = texts;
    return o;
}

bool fromJson(const QJsonObject &o, SavedState &s)
{
    s.collectClipboard = o["CollectClipboard"].toBool();
    s.collectTextClipboard = o["CollectTextClipboard"].toBool();
    s.autoHang = o["AutoHang"].toBool(true);
    s.hoverPreview = o["HoverPreview"].toBool(true);
    s.theme = o["Theme"].toString(QStringLiteral("classic"));
    s.displayDevice = o["DisplayDevice"].toString();
    s.lastUpdateNotification = o["LastUpdateNotification"].toString();
    s.captureModifiers = static_cast<unsigned>(o["CaptureModifiers"].toInt(3));
    s.captureKey = static_cast<unsigned>(o["CaptureKey"].toInt('S'));

    s.images.clear();
    for (const QJsonValue &v : o["Images"].toArray()) s.images.append(v.toString());
    s.collection.clear();
    for (const QJsonValue &v : o["Collection"].toArray()) s.collection.append(v.toString());
    s.texts.clear();
    for (const QJsonValue &v : o["Texts"].toArray())
        if (v.isObject()) s.texts.append(textFromJson(v.toObject()));
    return true;
}

} // namespace StateJson

StateStore::StateStore(const QString &rootDir)
    : root(QDir(rootDir).absolutePath())
    , inbox(root + "/Captures")
    , file_(root + "/state.json")
{
    QDir().mkpath(root);
    QDir().mkpath(inbox);
}

bool StateStore::load()
{
    QFile f(file_);
    if (!f.exists()) return true;                 // 首次运行用默认状态
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        logError(QStringLiteral("state.json 解析失败：") + err.errorString());
        return false;
    }
    return StateJson::fromJson(doc.object(), state);
}

bool StateStore::save()
{
    QDir().mkpath(root);
    QSaveFile f(file_);
    if (!f.open(QIODevice::WriteOnly)) { logError(QStringLiteral("无法写入 ") + file_); return false; }
    QJsonDocument doc(StateJson::toJson(state));
    f.write(doc.toJson(QJsonDocument::Compact));
    if (!f.commit()) { logError(QStringLiteral("无法提交 ") + file_); return false; }
    return true;
}

void StateStore::logError(const QString &message)
{
    QDir().mkpath(root);
    QFile f(root + "/error.log");
    if (f.open(QIODevice::Append | QIODevice::WriteOnly)) {
        f.write(QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8());
        f.write("  ");
        f.write(message.toUtf8());
        f.write("\n");
    }
}
