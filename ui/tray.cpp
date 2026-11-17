#include "tray.h"

#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QIcon>

#include "state.h"

TrayController::TrayController(StateStore *store, QObject *parent)
    : QObject(parent), store_(store)
{
    icon_ = new QSystemTrayIcon(this);
    icon_->setIcon(QIcon(QStringLiteral(":/app.ico")));
    icon_->setToolTip(tr("图绳 PicRope"));
    buildMenu();
    icon_->setContextMenu(menu_);
    connect(icon_, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason r) {
                if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick)
                    emit toggleRequested();
            });
}

TrayController::~TrayController()
{
    icon_->hide();
}

void TrayController::show()
{
    icon_->show();
}

void TrayController::buildMenu()
{
    menu_ = new QMenu;
    actToggle_ = menu_->addAction(tr("显示/收起"), this, &TrayController::toggleRequested);

    menu_->addAction(tr("图片篮子…"), this, &TrayController::basketRequested);
    menu_->addAction(tr("文字笔记…"), this, &TrayController::textsRequested);
    menu_->addSeparator();
    actClip_ = menu_->addAction(tr("剪贴板收集 · 图片"));
    actClip_->setCheckable(true);
    connect(actClip_, &QAction::toggled, this, &TrayController::collectClipboardChanged);

    actText_ = menu_->addAction(tr("剪贴板收集 · 文字"));
    actText_->setCheckable(true);
    connect(actText_, &QAction::toggled, this, &TrayController::collectTextClipboardChanged);

    // 勾选 = 已开启“鼠标移到屏幕顶部自动弹出挂绳”，与设置界面同一份状态并持久化
    actEdge_ = menu_->addAction(tr("自动弹出"));
    actEdge_->setCheckable(true);
    connect(actEdge_, &QAction::toggled, this, [this](bool on){
        emit edgePopupChanged(on);
    });

    actHang_ = menu_->addAction(tr("新图片自动上绳"));
    actHang_->setCheckable(true);
    connect(actHang_, &QAction::toggled, this, &TrayController::autoHangChanged);

    menu_->addSeparator();
    menu_->addAction(tr("设置…"), this, &TrayController::settingsRequested);

    QMenu *more = menu_->addMenu(tr("更多"));
    actCapture_ = more->addAction(tr("区域截图…"), this, &TrayController::captureRequested);
    more->addAction(tr("打开收集文件夹"), this, &TrayController::openFolderRequested);
    more->addAction(tr("全部取下（保留文件）"), this, &TrayController::clearShelfRequested);

    menu_->addSeparator();
    menu_->addAction(tr("退出图绳"), this, &TrayController::exitRequested);

    refresh();
}

void TrayController::refresh()
{
    if (!menu_) return;
    // 刷新时阻断信号，避免勾选状态回写触发 edgePopupChanged 形成回环
    const QSignalBlocker blockEdge(actEdge_);
    actClip_->setChecked(store_->state.collectClipboard);
    actText_->setChecked(store_->state.collectTextClipboard);
    actHang_->setChecked(store_->state.autoHang);
    actEdge_->setChecked(store_->state.edgePopup);   // 勾选 = 已开启自动弹出
}
