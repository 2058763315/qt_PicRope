#pragma once
// =====================================================================
//  core/texts.h — 文字收藏（对应原 PicRope 的 TextCollection）
// =====================================================================

#include <QObject>
#include <QVector>

#include "state.h"

class TextCollection : public QObject
{
    Q_OBJECT
public:
    explicit TextCollection(StateStore *store, QObject *parent = nullptr)
        : QObject(parent), store_(store) {}

    StateStore *store() const { return store_; }
    QVector<TextItem> &items() { return store_->state.texts; }
    const QVector<TextItem> &items() const { return store_->state.texts; }

    bool collectClipboard() const { return store_->state.collectTextClipboard; }
    void setCollectClipboard(bool on);

    TextItem *add(const QString &content);   // 去重后插入头部
    void remove(TextItem *item);
    void removeMany(const QVector<TextItem *> &selected);
    void clear();

signals:
    void changed();

private:
    StateStore *store_;
};
