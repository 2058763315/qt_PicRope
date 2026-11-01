#pragma once
// =====================================================================
//  system/win32.h — Windows 系统能力封装（热键 / 显示器 / 窗口 / 剪贴板辅助）
//  对应原 PicRope 的 WindowsNative.cs + WindowsEvents.cs
// =====================================================================

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QList>
#include <QAbstractNativeEventFilter>

class QWindow;

namespace Win32 {

    // 一块显示器的几何信息（逻辑坐标）
    struct Display {
        QString deviceName;
        QRect   bounds;     // 整块屏幕边界
        QRect   workArea;   // 去掉任务栏后的工作区
        bool    primary = false;
        double  scale = 1.0;
    };

    // ---- 显示器 ----
    QList<Display> screens();
    Display screenAt(const QPoint &cursor);               // 光标所在屏
    Display screenByName(const QString &deviceName);      // 按设备名找，找不到回主屏
    Display primaryScreen();
    double  scaleFactor(const QString &deviceName);

    // ---- 输入 / 光标 ----
    QPoint cursorPos();
    bool   asyncKeyDown(int key);   // GetAsyncKeyState

    // ---- 系统路径 / 剪贴板 ----
    QString screenshotsFolder();    // SHGetKnownFolderPath(Screenshots)
    QString clipboardOwnerName();   // 当前剪贴板拥有者进程名

    // ---- 窗口工具 ----
    void prepareOverlay(QWindow *w);              // 置顶工具窗口（不抢焦点、不进任务栏）
    void setMousePassthrough(QWindow *w, bool pass); // WS_EX_TRANSPARENT
    void placeTopmost(QWindow *w, const QRect &geom); // SetWindowPos 置顶定位

    // ---- 状态检测 ----
    bool foregroundFullscreen(QWindow *overlay);  // 前台窗口是否铺满当前屏
    bool overCaptionButtons(QWindow *overlay);    // 光标是否压在最大化窗口的标题按钮上
    QString windowTitle(void *hwnd);
}

// =====================================================================
//  SystemEvents — 捕获全局热键（WM_HOTKEY）的 NativeEventFilter
// =====================================================================
class SystemEvents : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit SystemEvents(QObject *parent = nullptr);
    ~SystemEvents() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

    // 显示/收起热键（固定 Ctrl+Alt+T）
    bool registerToggleHotkey();
    void unregisterToggleHotkey();
    bool toggleAvailable() const { return toggleOk_; }

    // 区域截图热键（可自定义组合）
    bool registerCaptureHotkey(unsigned mods, unsigned key);
    void unregisterCaptureHotkey();
    bool captureAvailable() const { return captureOk_; }

    // 最近一次 RegisterHotKey 失败时的 GetLastError（0 表示成功）
    unsigned lastError() const { return lastErr_; }

signals:
    void toggleRequested();
    void captureRequested();
    void exitRequested();

private:
    int  toggleId_ = 1;
    int  captureId_ = 2;
    bool toggleOk_ = false;
    bool captureOk_ = false;
    unsigned lastErr_ = 0;
    unsigned captureMods_ = 0;
    unsigned captureKey_ = 0;
};
