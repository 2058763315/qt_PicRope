#pragma once
// =====================================================================
//  app.h — 桌面控制器（对应原 PicRope 的 App.cs DesktopController）
//  装配所有模块、处理动作、监听剪贴板与托盘/快捷键
// =====================================================================

#include <QObject>
#include <QPointer>
#include <QVector>

class StateStore;
class Gallery;
class TextCollection;
class CapturePipeline;
class ShelfWindow;
class TrayController;
class SystemEvents;
class PinnedImageWindow;
class BasketWindow;
class TextsWindow;
class PanelWindow;
class CaptureItem;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(StateStore *store, QObject *parent = nullptr);
    ~AppController() override;

    bool start();
    void stop();

private:
    void connectAll();
    void onClipboardChanged();

    void onCopy(CaptureItem *item);
    void onOpen(CaptureItem *item);
    void onEdit(CaptureItem *item);
    void onDetach(CaptureItem *item);
    void onTrash(CaptureItem *item);
    void onSave(CaptureItem *item);
    void onLocate(CaptureItem *item);
    void onBasket();
    void onTexts();
    void showPanelBesideDock(PanelWindow *panel);
    void syncShelfHold();
    void onCaptureShortcut();

    StateStore *store_;
    Gallery *gallery_ = nullptr;
    TextCollection *texts_ = nullptr;
    CapturePipeline *capture_ = nullptr;
    ShelfWindow *shelf_ = nullptr;
    TrayController *tray_ = nullptr;
    SystemEvents *events_ = nullptr;
    BasketWindow *basket_ = nullptr;
    TextsWindow *textsWin_ = nullptr;
    QVector<QPointer<PinnedImageWindow>> pins_;
    bool running_ = false;
};
