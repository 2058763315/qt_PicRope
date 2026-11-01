#include "win32.h"

#include <QGuiApplication>
#include <QCursor>
#include <QScreen>
#include <QWindow>
#include <QDir>

#include <windows.h>
#include <shellscalingapi.h>
#include <dwmapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <utility>
#include <algorithm>

// =====================================================================
//  显示器（基于 Qt QScreen 逻辑坐标，跨 DPI 更省心）
// =====================================================================
namespace {
inline QRect toRect(const QScreen *s) {
    if (!s) return QRect();
    return QRect(s->geometry().topLeft(), s->geometry().size());
}
inline QRect toWork(const QScreen *s) {
    if (!s) return QRect();
    return QRect(s->availableGeometry().topLeft(), s->availableGeometry().size());
}
inline Win32::Display fromScreen(QScreen *s) {
    Win32::Display d;
    d.deviceName = s->name();
    d.bounds = toRect(s);
    d.workArea = toWork(s);
    d.primary = (s == QGuiApplication::primaryScreen());
    d.scale = s->devicePixelRatio();
    return d;
}
HWND hwndOf(QWindow *w) { return w ? reinterpret_cast<HWND>(w->winId()) : nullptr; }
}

namespace Win32 {

QList<Display> screens()
{
    QList<Display> out;
    const auto list = QGuiApplication::screens();
    for (QScreen *s : list) out.append(fromScreen(s));
    return out;
}

Display screenAt(const QPoint &cursor)
{
    QScreen *s = QGuiApplication::screenAt(cursor);
    if (s) return fromScreen(s);
    return primaryScreen();
}

Display screenByName(const QString &deviceName)
{
    const auto list = QGuiApplication::screens();
    for (QScreen *s : list)
        if (s->name().compare(deviceName, Qt::CaseInsensitive) == 0)
            return fromScreen(s);
    return primaryScreen();
}

Display primaryScreen()
{
    return fromScreen(QGuiApplication::primaryScreen());
}

double scaleFactor(const QString &deviceName)
{
    return screenByName(deviceName).scale;
}

QPoint cursorPos()
{
    // 返回 Qt 逻辑坐标：高分屏下与窗口几何、QScreen 保持一致
    return QCursor::pos();
}

bool asyncKeyDown(int key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

QString screenshotsFolder()
{
    PWSTR path = nullptr;
    const GUID id = { 0xb7bede81, 0xdf94, 0x4682, { 0xa7, 0xd8, 0x57, 0xa5, 0x26, 0x20, 0xb8, 0x6f } };
    if (SHGetKnownFolderPath(id, 0, nullptr, &path) == S_OK && path) {
        QString result = QString::fromWCharArray(path);
        CoTaskMemFree(path);
        return result;
    }
    if (path) CoTaskMemFree(path);
    return QDir::homePath() + "/Pictures/Screenshots";
}

QString clipboardOwnerName()
{
    HWND owner = GetClipboardOwner();
    if (!owner) return QString();
    DWORD pid = 0;
    GetWindowThreadProcessId(owner, &pid);
    if (pid == 0) return QString();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return QString();
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);
    QString name;
    if (Process32FirstW(snap, &entry)) {
        do {
            if (entry.th32ProcessID == pid) { name = QString::fromWCharArray(entry.szExeFile); break; }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
    return name;
}

void prepareOverlay(QWindow *w)
{
    HWND h = hwndOf(w);
    if (!h) return;
    SetPropW(h, L"NonRudeHWND", reinterpret_cast<HANDLE>(1));
    LONG_PTR style = GetWindowLongPtrW(h, GWL_EXSTYLE);
    SetWindowLongPtrW(h, GWL_EXSTYLE, style | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
}

void setMousePassthrough(QWindow *w, bool pass)
{
    HWND h = hwndOf(w);
    if (!h) return;
    LONG_PTR style = GetWindowLongPtrW(h, GWL_EXSTYLE);
    LONG_PTR next = pass ? (style | WS_EX_TRANSPARENT) : (style & ~WS_EX_TRANSPARENT);
    if (next != style) SetWindowLongPtrW(h, GWL_EXSTYLE, next);
}

void placeTopmost(QWindow *w, const QRect &geom)
{
    Q_UNUSED(geom)
    HWND h = hwndOf(w);
    if (!h) return;
    // 只重新置顶，位置与大小交给 Qt（SetWindowPos 用物理像素，直接传逻辑几何会错位）
    SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

// ---- 检测前台窗口是否铺满某块屏幕 ----
bool foregroundFullscreen(QWindow *overlay)
{
    HWND overlayHwnd = hwndOf(overlay);
    HWND fg = GetForegroundWindow();
    if (!fg || fg == overlayHwnd || fg == GetShellWindow()) return false;

    WCHAR cls[128];
    GetClassNameW(fg, cls, 127);
    QString type = QString::fromWCharArray(cls);
    if (type == "WorkerW" || type == "Progman" || type.startsWith("Shell_")) return false;

    RECT r;
    if (!GetWindowRect(fg, &r)) return false;

    // 用窗口所在显示器的物理矩形比较，避免高分屏下逻辑/物理坐标混用
    HMONITOR monitor = MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) return false;
    const RECT &mb = mi.rcMonitor;
    return r.left <= mb.left && r.top <= mb.top && r.right >= mb.right && r.bottom >= mb.bottom;
}

// ---- 光标是否悬停在最大化窗口右上角（系统标题按钮区），避免绳子遮挡 ----
bool overCaptionButtons(QWindow *overlay)
{
    POINT pt;
    if (!GetCursorPos(&pt)) return false;

    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) return false;
    const RECT mb = mi.rcMonitor;

    double scale = 1.0;
    UINT dpiX = 96, dpiY = 96;
    if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) && dpiX > 0)
        scale = dpiX / 96.0;
    const int bandW = qRound(150 * scale);
    const int bandH = qRound(40 * scale);

    if (pt.x < mb.right - bandW || pt.y < mb.top || pt.y >= mb.top + bandH) return false;

    const HWND overlayHwnd = hwndOf(overlay);
    struct Ctx { HWND overlay; POINT cursor; HWND hit; };
    Ctx ctx{ overlayHwnd, pt, nullptr };
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto *ctx = reinterpret_cast<Ctx *>(lp);
        if (h == ctx->overlay || h == GetShellWindow() || !IsWindowVisible(h) || IsIconic(h)) return TRUE;
        LONG_PTR ex = GetWindowLongPtrW(h, GWL_EXSTYLE);
        if ((ex & WS_EX_TRANSPARENT) != 0) return TRUE;
        int cloaked = 0;
        if (DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)) == S_OK && cloaked != 0) return TRUE;
        RECT r;
        if (!GetWindowRect(h, &r)) return TRUE;
        WCHAR cls[128];
        GetClassNameW(h, cls, 127);
        QString type = QString::fromWCharArray(cls);
        if (type == "WorkerW" || type == "Progman" || type.startsWith("Shell_")) return TRUE;
        // 只看光标正下方的顶层窗口（EnumWindows 按 Z 序返回）
        if (ctx->cursor.x < r.left || ctx->cursor.x >= r.right
            || ctx->cursor.y < r.top || ctx->cursor.y >= r.bottom) return TRUE;
        ctx->hit = h;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&ctx));
    if (!ctx.hit) return false;

    RECT r;
    if (!GetWindowRect(ctx.hit, &r)) return false;
    // 只有最大化（覆盖整块屏幕）的窗口，右上角才是系统标题按钮
    if (r.left > mb.left || r.top > mb.top || r.right < mb.right || r.bottom < mb.bottom) return false;
    if (r.right - r.left < 280 || r.bottom - r.top < 80) return false;

    const int right = qMin<LONG>(r.right, mb.right);
    const int top = qMax<LONG>(r.top, mb.top);
    return pt.x >= right - bandW && pt.x < right
        && pt.y >= top && pt.y < top + bandH;
}

QString windowTitle(void *hwnd)
{
    if (!hwnd) return QString();
    WCHAR buf[1024];
    GetWindowTextW(reinterpret_cast<HWND>(hwnd), buf, 1024);
    return QString::fromWCharArray(buf);
}

} // namespace Win32

// =====================================================================
//  SystemEvents — 全局热键
// =====================================================================
SystemEvents::SystemEvents(QObject *parent) : QObject(parent)
{
    qApp->installNativeEventFilter(this);
}

SystemEvents::~SystemEvents()
{
    unregisterToggleHotkey();
    unregisterCaptureHotkey();
    qApp->removeNativeEventFilter(this);
}

bool SystemEvents::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result)
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG") return false;
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY) {
        const int id = static_cast<int>(msg->wParam);
        if (id == toggleId_) { emit toggleRequested(); return true; }
        if (id == captureId_) { emit captureRequested(); return true; }
    }
    return false;
}

bool SystemEvents::registerToggleHotkey()
{
    unregisterToggleHotkey();
    // MOD_CONTROL(2) | MOD_ALT(1) = 3，key = 'T'
    toggleOk_ = RegisterHotKey(nullptr, toggleId_, MOD_CONTROL | MOD_ALT, 'T') != FALSE;
    lastErr_ = toggleOk_ ? 0 : GetLastError();
    return toggleOk_;
}

void SystemEvents::unregisterToggleHotkey()
{
    if (toggleOk_) { UnregisterHotKey(nullptr, toggleId_); toggleOk_ = false; }
}

bool SystemEvents::registerCaptureHotkey(unsigned mods, unsigned key)
{
    if (mods == 0 || (mods & ~15u) != 0 || key < 0x20 || key > 0xFE) return false;
    unregisterCaptureHotkey();
    // MOD_NOREPEAT = 0x4000
    captureOk_ = RegisterHotKey(nullptr, captureId_, mods | 0x4000, key) != FALSE;
    lastErr_ = captureOk_ ? 0 : GetLastError();
    if (captureOk_) { captureMods_ = mods; captureKey_ = key; }
    return captureOk_;
}

void SystemEvents::unregisterCaptureHotkey()
{
    if (captureOk_) { UnregisterHotKey(nullptr, captureId_); captureOk_ = false; }
}
