#pragma once
// =====================================================================
//  core/gallery.h — 图片收藏管理（对应原 PicRope 的 Gallery + CaptureItem）
//  - items      ：挂绳上的图片（最近 50）
//  - collected  ：已收集的全部图片（含篮子中待挂的）
//
//  为了让卡片、贴图窗口在列表增删时不会持有失效指针，条目统一用
//  QSharedPointer 管理：两个列表共享同一对象，界面侧也能安全延长生命周期。
// =====================================================================

#include <QObject>
#include <QVector>
#include <QSharedPointer>
#include <QStringList>
#include <QDateTime>
#include <QFileInfo>

#include "state.h"
#include "image.h"

// 一张收藏的图片
class CaptureItem
{
public:
    CaptureItem() = default;
    CaptureItem(const QString &path, const ImageContent &img)
        : path(path), image(img), written(QFileInfo(path).lastModified()) {}

    QString path;
    ImageContent image;
    QDateTime written;
};

class Gallery : public QObject
{
    Q_OBJECT
public:
    using ItemPtr = QSharedPointer<CaptureItem>;

    explicit Gallery(StateStore *store, QObject *parent = nullptr);

    StateStore *store() const { return store_; }

    QVector<ItemPtr> items;        // 挂绳（顺序 = 展示顺序）
    QVector<ItemPtr> collected;    // 全部收集

    bool contains(const QString &path) const;
    bool containsInCollected(const QString &path) const;

    // 收进篮子（不进挂绳）；已存在则返回原项
    ItemPtr collect(const QString &path, const ImageContent &img);
    // 挂上绳子；已存在则忽略
    ItemPtr add(const QString &path, const ImageContent &img);

    // 按原始指针找到共享条目（贴图窗口需要延长生命周期）
    ItemPtr ptrOf(const CaptureItem *item) const;
    // 按路径查找（先挂绳后篮子）
    ItemPtr byPath(const QString &path) const;

    void remove(CaptureItem *item);              // 仅从挂绳取下
    void removeMany(const QVector<CaptureItem *> &targets);
    void clear();
    void forget(CaptureItem *item);              // 挂绳与收集同时移除记录（不删文件）
    ItemPtr replace(CaptureItem *item, const QString &newPath); // 编辑器换副本
    void prune();                                // 清理已不存在文件

    // 从磁盘刷新某张图片（共享对象，挂绳与篮子同步更新）
    bool refresh(CaptureItem *item);

    void notifyChanged();

    static bool samePath(const QString &a, const QString &b);

signals:
    void changed();

private:
    void syncStateFromStore();         // 由 state.images/collection 重建
    void saveToStore();
    int indexOf(const QVector<ItemPtr> &list, const QString &path) const;
    StateStore *store_;
};
