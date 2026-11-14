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
    icon_->setToolTip(QStringLiteral("图绳 PicRope"));
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
    actToggle_ = menu_->addAction(QStringLiteral("显示/收起"), this, &TrayController::toggleRequested);

    // 勾选即临时关闭“鼠标移到屏幕顶部自动展开”，重启后恢复
    actEdge_ = menu_->addAction(QStringLiteral("关闭自动弹出"));
    actEdge_->setCheckable(true);
    connect(actEdge_, &QAction::toggled, this, [this](bool off){
        emit edgePopupChanged(!off);
    });

    menu_->addAction(QStringLiteral("图片篮子…"), this, &TrayController::basketRequested);
    menu_->addAction(QStringLiteral("文字笔记…"), this, &TrayController::textsRequested);
    menu_->addSeparator();
    actClip_ = menu_->addAction(QStringLiteral("剪贴板收集 · 图片"));
    actClip_->setCheckable(true);
    connect(actClip_, &QAction::toggled, this, &TrayController::collectClipboardChanged);

    actText_ = menu_->addAction(QStringLiteral("剪贴板收集 · 文字"));
    actText_->setCheckable(true);
    connect(actText_, &QAction::toggled, this, &TrayController::collectTextClipboardChanged);

    actHang_ = menu_->addAction(QStringLiteral("新图片自动上绳"));
    actHang_->setCheckable(true);
    connect(actHang_, &QAction::toggled, this, &TrayController::autoHangChanged);

    menu_->addSeparator();
    menu_->addAction(QStringLiteral("设置…"), this, &TrayController::settingsRequested);

    QMenu *more = menu_->addMenu(QStringLiteral("更多"));
    actCapture_ = more->addAction(QStringLiteral("区域截图…"), this, &TrayController::captureRequested);
    more->addAction(QStringLiteral("打开收集文件夹"), this, &TrayController::openFolderRequested);
    more->addAction(QStringLiteral("全部取下（保留文件）"), this, &TrayController::clearShelfRequested);

    menu_->addSeparator();
    menu_->addAction(QStringLiteral("退出图绳"), this, &TrayController::exitRequested);

    refresh();
}

void TrayController::refresh()
{
    if (!menu_) return;
    actClip_->setChecked(store_->state.collectClipboard);
    actText_->setChecked(store_->state.collectTextClipboard);
    actHang_->setChecked(store_->state.autoHang);
}
