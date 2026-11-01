#pragma once
// =====================================================================
//  ui/tray.h — 系统托盘（对应原 PicRope 的 TrayPanel）
// =====================================================================

#include <QObject>
#include <QSystemTrayIcon>

class QMenu;
class QAction;
class QActionGroup;
class StateStore;

class TrayController : public QObject
{
    Q_OBJECT
public:
    explicit TrayController(StateStore *store, QObject *parent = nullptr);
    ~TrayController() override;

    void show();
    void refresh();                       // 依据 StateStore 刷新开关状态
    QSystemTrayIcon *icon() const { return icon_; }

signals:
    void toggleRequested();
    void basketRequested();
    void textsRequested();
    void collectClipboardChanged(bool on);
    void collectTextClipboardChanged(bool on);
    void autoHangChanged(bool on);
    void themeSelected(const QString &id);
    void captureRequested();
    void openFolderRequested();
    void clearShelfRequested();
    void exitRequested();

private:
    void buildMenu();

    StateStore *store_;
    QSystemTrayIcon *icon_;
    QMenu *menu_;

    QAction *actToggle_ = nullptr;
    QAction *actClip_ = nullptr;
    QAction *actText_ = nullptr;
    QAction *actHang_ = nullptr;
    QAction *actCapture_ = nullptr;
    QMenu *themeMenu_ = nullptr;
};
