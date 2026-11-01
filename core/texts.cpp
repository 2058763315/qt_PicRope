#include "texts.h"

#include <QUuid>
#include <QDateTime>
#include <QSet>

void TextCollection::setCollectClipboard(bool on)
{
    if (store_->state.collectTextClipboard == on) return;
    store_->state.collectTextClipboard = on;
    store_->save();
    emit changed();
}

TextItem *TextCollection::add(const QString &content)
{
    if (content.trimmed().isEmpty()) return nullptr;
    // 去重
    for (TextItem &t : store_->state.texts)
        if (t.content == content) return &t;
    TextItem t;
    t.id = QUuid::createUuid().toString(QUuid::WithoutBraces).replace("-", "");
    t.content = content;
    t.createdUtc = QDateTime::currentDateTimeUtc();
    store_->state.texts.prepend(t);
    store_->save();
    emit changed();
    return &store_->state.texts[0];
}

void TextCollection::remove(TextItem *item)
{
    auto &list = store_->state.texts;
    for (int i = 0; i < list.size(); ++i) {
        if (&list[i] == item || list[i].id == item->id) {
            list.removeAt(i);
            store_->save();
            emit changed();
            return;
        }
    }
}

void TextCollection::removeMany(const QVector<TextItem *> &selected)
{
    QSet<QString> ids;
    for (TextItem *t : selected) ids.insert(t->id);
    auto &list = store_->state.texts;
    QVector<TextItem> kept;
    for (const TextItem &t : list)
        if (!ids.contains(t.id)) kept.append(t);
    if (kept.size() == list.size()) return;
    list = kept;
    store_->save();
    emit changed();
}

void TextCollection::clear()
{
    if (store_->state.texts.isEmpty()) return;
    store_->state.texts.clear();
    store_->save();
    emit changed();
}
