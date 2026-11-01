#pragma once
// =====================================================================
//  ui/card.h — 挂绳上的单张图片卡片（对应原 PicRope 的 ShotCard）
//  交互：单击复制、双击打开、长按编辑、右键菜单、拖拽、点夹子贴桌面
// =====================================================================

#include <QWidget>
#include <QPoint>
#include <QElapsedTimer>

#include "gallery.h"
#include "themes.h"

class QTimer;
class QPaintEvent;
class QMouseEvent;
class QContextMenuEvent;
class QEnterEvent;

class ShotCard : public QWidget
{
    Q_OBJECT
public:
    explicit ShotCard(Gallery *gallery, const Gallery::ItemPtr &item,
                      int colorIndex = 0, QWidget *parent = nullptr);

    CaptureItem *item() const { return item_.data(); }
    Gallery::ItemPtr itemPtr() const { return item_; }

    void setTheme(const LineTheme &t);
    void flashCopied();
    void setHovered(bool on);       // 由绳子窗口轮询驱动（窗口点击穿透时也能正确显示悬停）

    QSize sizeHint() const override;
    static constexpr int cardW = 170;
    static constexpr int cardH = 178;

    enum Batch { BatchLeft = 0, BatchRight, BatchOthers, BatchAll, BatchSelf };

signals:
    void copyRequested();
    void openRequested();
    void editRequested();
    void detachRequested();           // 贴到桌面
    void trashRequested();            // 删除（移回收站）
    void saveRequested();             // 保存副本
    void locateRequested();           // 在文件夹中显示
    void batchRequested(Batch batch);
    void dragActive(bool active);     // 拖拽进行中（绳子暂停自动收起）

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;

private:
    QSizeF photoSize() const;
    QRectF surfaceRect() const;
    QRectF imageRect() const;
    QRectF clipRect() const;
    QRectF closeRect() const;
    bool clickOnClose(const QPoint &p) const;

    Gallery *gallery_;
    Gallery::ItemPtr item_;
    LineTheme theme_;
    int colorIndex_ = 0;
    double tilt_ = 0.0;

    bool hover_ = false;
    bool hoverClose_ = false;
    bool pressed_ = false;
    bool held_ = false;          // 长按已触发
    bool moved_ = false;         // 已开始拖拽
    bool copiedFlash_ = false;
    qreal flashOpacity_ = 0.0;

    QPoint pressPos_;
    QElapsedTimer pressTime_;
    QTimer *flashTimer_ = nullptr;
    QTimer *longPressTimer_ = nullptr;
    QTimer *clickTimer_ = nullptr;
};
