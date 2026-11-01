#pragma once
// =====================================================================
//  ui/pinned.h — 桌面贴图（对应原 PicRope 的 PinnedImageWindow）
//  无边框置顶窗口：显示原图，可拖动、滚轮缩放；点图钉挂回绳子。
// =====================================================================

#include <QWidget>
#include <QPoint>
#include <QImage>
#include <QPixmap>

#include "gallery.h"

class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QShowEvent;
class QKeyEvent;

class PinnedImageWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PinnedImageWindow(const Gallery::ItemPtr &item, QWidget *parent = nullptr);

    CaptureItem *item() const { return item_.data(); }
    Gallery::ItemPtr itemPtr() const { return item_; }

    void placeAtScreen(const QRect &screenGeom);

signals:
    void closed();                 // 图钉收起，由持有者决定是否回挂
    void removeRequested();        // 关闭并删除（若需要）

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void showEvent(QShowEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    void loadImage();
    void applyLayout(bool keepCenter);
    QRectF imageRect() const;
    QRectF pinRect() const;
    QRectF closeRect() const;
    QRectF barRect() const;
    QString titleText() const;
    QSize imageDisplaySize() const;

    Gallery::ItemPtr item_;
    QImage original_;
    QPixmap display_;
    double fit_ = 1.0;
    double zoom_ = 1.0;
    QPoint dragOffset_;
    bool dragging_ = false;
    bool overPin_ = false;
    bool overClose_ = false;
};
