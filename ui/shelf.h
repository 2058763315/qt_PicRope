#pragma once
// =====================================================================
//  ui/shelf.h — 绳子主窗口（对应原 PicRope 的 ShelfWindow）
//  置顶透明窄条：鼠标移到屏幕上沿自动展开，离开后收起；
//  在非交互区域点击会穿透到下层窗口，不遮挡标题栏。
// =====================================================================

#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QVector>

#include "themes.h"
#include "gallery.h"

class QHBoxLayout;
class QScrollArea;
class QPropertyAnimation;
class QTimer;
class QPaintEvent;
class QShowEvent;
class QHideEvent;
class QWheelEvent;
class StateStore;
class CaptureItem;
class ShotCard;
class ShelfButton;

class ShelfWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ShelfWindow(Gallery *gallery, StateStore *store, QWidget *parent = nullptr);

    void rebuild();                     // 依 gallery 重建卡片
    void setTheme(const LineTheme &t);
    LineTheme theme() const { return theme_; }

    void setScreenGeom(const QRect &geom);
    void slideIn(int peekMs = 2200);    // 展开（peekMs 内不自动收起，0 表示不额外停留）
    void slideOut();
    bool toggleShelf();
    bool isShownNow() const { return shown_; }
    void forceHidden();                 // 托盘/退出时强制收起

    QScrollArea *scrollArea() const { return scroll_; }
    void refreshBadges() { updateBadges(); }

    QRect toolDockRect() const;         // 三个工具按钮在屏幕上的矩形
    void setHoldOpen(bool hold);        // 面板打开期间保持展开，不自动收起

signals:
    void copyRequested(CaptureItem *item);
    void openRequested(CaptureItem *item);
    void editRequested(CaptureItem *item);
    void detachRequested(CaptureItem *item);
    void trashRequested(CaptureItem *item);
    void saveRequested(CaptureItem *item);
    void locateRequested(CaptureItem *item);
    void batchRequested(int batch);
    void basketRequested();
    void textsRequested();
    void retracted();                   // 绳子收起（手动/热键/让位），面板应一并隐藏

protected:
    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;

private:
    void startEdgeDetector();
    void edgeTick();
    ShotCard *makeCard(const Gallery::ItemPtr &item, int index);
    void placeOnScreen();
    void animateTo(const QPoint &dest, bool hideAfter);
    bool overInteractive() const;
    void updateBadges();

    Gallery *gallery_;
    StateStore *store_;
    LineTheme theme_;
    QRect screenGeom_;
    QRect placedGeom_;

    QWidget *container_ = nullptr;
    QHBoxLayout *layout_ = nullptr;
    QScrollArea *scroll_ = nullptr;
    ShelfButton *basketBtn_ = nullptr;
    ShelfButton *textsBtn_ = nullptr;
    ShelfButton *hideBtn_ = nullptr;
    QPropertyAnimation *anim_ = nullptr;
    QTimer *edgeTimer_ = nullptr;
    QVector<ShotCard *> cards_;

    bool shown_ = false;
    bool pinned_ = false;            // 手动展开，保持到再次切换
    bool hideAfterAnim_ = false;
    bool passState_ = false;
    bool suppressEdge_ = false;
    bool avoidingCaption_ = false;
    bool prevDown_ = false;
    bool dragActive_ = false;        // 卡片正在拖出，暂停自动收起
    bool holdOpen_ = false;          // 面板打开期间强制保持展开
    qint64 edgeSince_ = -1;
    qint64 awaySince_ = -1;
    qint64 peekUntil_ = -1;
};
