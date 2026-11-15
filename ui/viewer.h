#pragma once
// =====================================================================
//  ui/viewer.h — 内置图片查看器
//  固定尺寸窗口：同目录图片 ← / → 切换，滚轮缩放，Esc 关闭。
//  底部信息栏可拖动窗口，右上角有关闭 / 最小化按钮。
// =====================================================================

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QPointF>
#include <QStringList>

class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

class ImageViewer : public QWidget
{
    Q_OBJECT
public:
    explicit ImageViewer(QWidget *parent = nullptr);

    // 打开指定图片，并收集其所在目录的同类图片作为浏览列表
    void openFile(const QString &path);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    void loadDirectory(const QString &path);
    void showCurrent();
    void step(int delta);
    void rebuildDisplay();
    void clampPan();
    double currentScale() const;

    QRectF imageRect() const;
    QRectF barRect() const;
    QRectF prevRect() const;
    QRectF nextRect() const;
    QRectF closeRect() const;
    QRectF minimizeRect() const;

    QImage image_;
    QPixmap display_;
    QStringList files_;
    int index_ = -1;
    QString currentPath_;
    double dpr_ = 1.0;

    double fit_ = 1.0;        // 适配窗口的基准比例
    double zoom_ = 1.0;       // 在 fit_ 基础上的缩放倍数
    QPointF pan_;             // 放大后图片的平移量

    bool sized_ = false;
    bool movingWindow_ = false;
    bool panning_ = false;
    QPoint dragOffset_;
    QPointF dragStart_;
    QPointF panStart_;
    bool overPrev_ = false;
    bool overNext_ = false;
    bool overClose_ = false;
    bool overMin_ = false;
};
