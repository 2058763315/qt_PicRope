#include "gallery.h"

#include <QFileInfo>
#include <QDir>
#include <QSet>

bool Gallery::samePath(const QString &a, const QString &b)
{
    return QFileInfo(a).absoluteFilePath().compare(
               QFileInfo(b).absoluteFilePath(), Qt::CaseInsensitive) == 0;
}

Gallery::Gallery(StateStore *store, QObject *parent)
    : QObject(parent), store_(store)
{
    syncStateFromStore();
}

bool Gallery::contains(const QString &path) const
{
    for (const ItemPtr &i : items)
        if (samePath(i->path, path)) return true;
    return false;
}

bool Gallery::containsInCollected(const QString &path) const
{
    for (const ItemPtr &i : collected)
        if (samePath(i->path, path)) return true;
    return false;
}

int Gallery::indexOf(const QVector<ItemPtr> &list, const QString &path) const
{
    for (int i = 0; i < list.size(); ++i)
        if (samePath(list[i]->path, path)) return i;
    return -1;
}

Gallery::ItemPtr Gallery::ptrOf(const CaptureItem *item) const
{
    if (!item) return ItemPtr();
    for (const ItemPtr &p : items)
        if (p.data() == item || samePath(p->path, item->path)) return p;
    for (const ItemPtr &p : collected)
        if (p.data() == item || samePath(p->path, item->path)) return p;
    return ItemPtr();
}

Gallery::ItemPtr Gallery::byPath(const QString &path) const
{
    int i = indexOf(items, path);
    if (i >= 0) return items[i];
    i = indexOf(collected, path);
    if (i >= 0) return collected[i];
    return ItemPtr();
}

Gallery::ItemPtr Gallery::collect(const QString &path, const ImageContent &img)
{
    int idx = indexOf(collected, path);
    if (idx >= 0) {
        ItemPtr existing = collected[idx];
        if (img.valid()) {
            existing->image = img;
            existing->written = QFileInfo(path).lastModified();
        }
        return existing;
    }
    ItemPtr item(new CaptureItem(path, img));
    collected.append(item);
    saveToStore();
    notifyChanged();
    return item;
}

Gallery::ItemPtr Gallery::add(const QString &path, const ImageContent &img)
{
    ItemPtr item;
    int ci = indexOf(collected, path);
    if (ci < 0) {
        item = ItemPtr(new CaptureItem(path, img));
        collected.append(item);
    } else {
        item = collected[ci];
        if (img.valid() && !img.thumbnail.isNull()) {
            item->image = img;
            item->written = QFileInfo(path).lastModified();
        }
    }

    if (indexOf(items, path) >= 0) return item;    // 已在绳上

    items.append(item);
    while (items.size() > 50) items.removeFirst();
    saveToStore();
    notifyChanged();
    return item;
}

void Gallery::remove(CaptureItem *item)
{
    if (!item) return;
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].data() == item || samePath(items[i]->path, item->path)) {
            items.removeAt(i);
            saveToStore();
            notifyChanged();
            return;
        }
    }
}

void Gallery::removeMany(const QVector<CaptureItem *> &targets)
{
    if (targets.isEmpty()) return;
    QSet<QString> paths;
    for (CaptureItem *t : targets)
        if (t) paths.insert(QFileInfo(t->path).absoluteFilePath().toLower());

    QVector<ItemPtr> kept;
    for (const ItemPtr &i : items)
        if (!paths.contains(QFileInfo(i->path).absoluteFilePath().toLower())) kept.append(i);
    if (kept.size() == items.size()) return;
    items = kept;
    saveToStore();
    notifyChanged();
}

void Gallery::clear()
{
    if (items.isEmpty()) return;
    items.clear();
    saveToStore();
    notifyChanged();
}

void Gallery::forget(CaptureItem *item)
{
    if (!item) return;
    bool changed = false;
    for (int pass = 0; pass < 2; ++pass) {
        QVector<ItemPtr> &list = pass == 0 ? items : collected;
        for (int i = 0; i < list.size(); ++i) {
            if (list[i].data() == item || samePath(list[i]->path, item->path)) {
                list.removeAt(i);
                changed = true;
                break;
            }
        }
    }
    if (!changed) return;
    saveToStore();
    notifyChanged();
}

Gallery::ItemPtr Gallery::replace(CaptureItem *item, const QString &newPath)
{
    if (!item) return ItemPtr();
    for (const ItemPtr &it : collected) {
        if (it.data() == item || samePath(it->path, item->path)) {
            it->path = newPath;
            it->written = QFileInfo(newPath).lastModified();
            saveToStore();
            notifyChanged();
            return it;
        }
    }
    return ItemPtr();
}

void Gallery::prune()
{
    bool changed = false;
    auto dropMissing = [&](QVector<ItemPtr> &list) {
        for (int i = list.size() - 1; i >= 0; --i) {
            if (!QFileInfo::exists(list[i]->path)) { list.removeAt(i); changed = true; }
        }
    };
    dropMissing(items);
    dropMissing(collected);
    if (changed) { saveToStore(); notifyChanged(); }
}

bool Gallery::refresh(CaptureItem *item)
{
    if (!item) return false;
    ImageContent img = ImageContent::read(item->path, false);
    if (!img.valid()) return false;
    item->image = img;
    item->written = QFileInfo(item->path).lastModified();
    return true;
}

void Gallery::notifyChanged() { emit changed(); }

void Gallery::saveToStore()
{
    QStringList imgs, coll;
    for (const ItemPtr &i : items) imgs.append(i->path);
    for (const ItemPtr &i : collected) coll.append(i->path);
    store_->state.images = imgs;
    store_->state.collection = coll;
    store_->save();
}

void Gallery::syncStateFromStore()
{
    items.clear();
    collected.clear();

    // 先按 collection 建立收集表
    for (const QString &p : store_->state.collection) {
        if (indexOf(collected, p) >= 0) continue;
        ImageContent img = ImageContent::read(p, false);
        if (!img.valid()) continue;
        collected.append(ItemPtr(new CaptureItem(p, img)));
    }

    // 再按 images 挂绳（引用收集表中的同一对象）
    for (const QString &p : store_->state.images) {
        if (indexOf(items, p) >= 0) continue;
        int ci = indexOf(collected, p);
        if (ci < 0) {
            ImageContent img = ImageContent::read(p, false);
            if (!img.valid()) continue;
            ItemPtr item(new CaptureItem(p, img));
            collected.append(item);
            items.append(item);
        } else {
            items.append(collected[ci]);
        }
    }
    while (items.size() > 50) items.removeFirst();
}
