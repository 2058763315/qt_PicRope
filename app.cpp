#include "app.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QTimer>
#include <QStringList>

#include "state.h"
#include "gallery.h"
#include "texts.h"
#include "capture.h"
#include "shelf.h"
#include "panels.h"
#include "settings.h"
#include "viewer.h"
#include "tray.h"
#include "updater.h"
#include "win32.h"
#include "clipboard.h"

AppController::AppController(StateStore *store, QObject *parent)
    : QObject(parent), store_(store)
{
    gallery_ = new Gallery(store_, this);
    texts_ = new TextCollection(store_, this);
    capture_ = new CapturePipeline(gallery_, Win32::screenshotsFolder(), this);
    shelf_ = new ShelfWindow(gallery_, store_);
    tray_ = new TrayController(store_, this);
    events_ = new SystemEvents(this);

    connectAll();
}

AppController::~AppController()
{
    stop();
}

bool AppController::start()
{
    if (running_) return true;

    // 收集管线
    capture_->start();

    // 托盘
    tray_->show();

    // 绳子：固定屏幕或跟随鼠标所在屏，滑入
    QRect shelfGeom = Win32::screenAt(Win32::cursorPos()).bounds;
    if (!store_->state.displayDevice.isEmpty())
        shelfGeom = Win32::screenByName(store_->state.displayDevice).bounds;
    shelf_->setScreenGeom(shelfGeom);
    shelf_->setEdgePopupEnabled(store_->state.edgePopup);   // 沿用上次的顶部自动弹出设置
    shelf_->slideIn(3500);

    // 启动后空闲时预热面板（图片篮子 / 文字笔记），首次打开不再卡顿
    QTimer::singleShot(400, this, [this]{ ensurePanels(); });

    // 自动检查更新：静默请求 GitHub 最新发行版，有新版则下载安装包并拉起安装
    if (store_->state.autoCheckUpdate) {
        QTimer::singleShot(4000, this, [this]{
            if (updater_) return;
            updater_ = new UpdateChecker(this);

            connect(updater_, &UpdateChecker::updateAvailable, this,
                    [this](const QString &version, const QUrl &setupUrl, const QUrl &pageUrl){
                if (setupUrl.isEmpty()) {
                    if (!pageUrl.isEmpty()) QDesktopServices::openUrl(pageUrl);
                    return;
                }

                // 先询问用户，同意后才下载并安装
                QMessageBox box(QMessageBox::Question, tr("发现新版本"),
                                tr("检测到新版本 %1，是否下载并自动安装？").arg(version),
                                QMessageBox::Yes | QMessageBox::No);
                box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
                box.setDefaultButton(QMessageBox::Yes);
                box.setInformativeText(
                    tr("下载完成后会自动启动安装程序，安装期间本软件将退出。"));
                if (box.exec() != QMessageBox::Yes) {
                    store_->logError(QStringLiteral("自动更新：用户取消下载"));
                    return;
                }

                store_->logError(QStringLiteral("自动更新：开始下载安装包"));
                updater_->download(setupUrl);
            });
            connect(updater_, &UpdateChecker::downloadFinished, this,
                    [this](const QString &path){
                store_->logError(QStringLiteral("自动更新：下载完成，启动安装程序"));
                if (!QProcess::startDetached(path)) {
                    store_->logError(QStringLiteral("自动更新：安装程序启动失败 ") + path);
                    return;
                }
                stop();
                QCoreApplication::quit();
            });
            connect(updater_, &UpdateChecker::failed, this, [this](const QString &msg){
                store_->logError(QStringLiteral("自动更新：") + msg);
            });
            updater_->checkLatest();
        });
    }

    // 全局热键：等事件循环启动后再注册（更稳），并记录 Windows 错误码
    QTimer::singleShot(0, this, [this]{
        if (!events_->registerToggleHotkey(store_->state.toggleHotkey))
            store_->logError(QStringLiteral("%1 热键注册失败（错误码 %2），"
                                            "可能已被其它程序占用")
                             .arg(store_->state.toggleHotkey)
                             .arg(events_->lastError()));
        else
            activeHotkey_ = store_->state.toggleHotkey;
        if (!events_->registerCaptureHotkey(store_->state.captureModifiers,
                                            store_->state.captureKey))
            store_->logError(QStringLiteral("区域截图热键注册失败（错误码 %1）")
                             .arg(events_->lastError()));
    });

    running_ = true;
    return true;
}

void AppController::stop()
{
    if (!running_) return;
    running_ = false;
    if (shelf_) shelf_->forceHidden();
    events_->unregisterToggleHotkey();
    events_->unregisterCaptureHotkey();
    if (basket_) { basket_->hide(); basket_->deleteLater(); basket_ = nullptr; }
    if (textsWin_) { textsWin_->hide(); textsWin_->deleteLater(); textsWin_ = nullptr; }
    if (settings_) { settings_->hide(); settings_->deleteLater(); settings_ = nullptr; }
    if (viewer_) { viewer_->hide(); viewer_->deleteLater(); viewer_ = nullptr; }
}

void AppController::connectAll()
{
    // 剪贴板监听（图片 + 文字）
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &AppController::onClipboardChanged);

    // 收集到新图 → 刷新绳子
    connect(gallery_, &Gallery::changed, this, [this]{
        if (shelf_) shelf_->rebuild();
    });
    connect(texts_, &TextCollection::changed, this, [this]{
        if (shelf_) shelf_->refreshBadges();
    });
    connect(capture_, &CapturePipeline::arrived, this, [this](const QString &, bool){
        if (shelf_) shelf_->slideIn(2200);
    });

    // 卡片动作
    connect(shelf_, &ShelfWindow::copyRequested, this, &AppController::onCopy);
    connect(shelf_, &ShelfWindow::openRequested, this, &AppController::onOpen);
    connect(shelf_, &ShelfWindow::editRequested, this, &AppController::onEdit);
    connect(shelf_, &ShelfWindow::trashRequested, this, &AppController::onTrash);
    connect(shelf_, &ShelfWindow::saveRequested, this, &AppController::onSave);
    connect(shelf_, &ShelfWindow::locateRequested, this, &AppController::onLocate);
    connect(shelf_, &ShelfWindow::basketRequested, this, &AppController::onBasket);
    connect(shelf_, &ShelfWindow::textsRequested, this, &AppController::onTexts);
    connect(shelf_, &ShelfWindow::settingsRequested, this, &AppController::onSettings);
    // 绳子收起时面板随之隐藏，避免面板悬空
    connect(shelf_, &ShelfWindow::retracted, this, [this]{
        if (basket_) basket_->hide();
        if (textsWin_) textsWin_->hide();
    });

    // 快捷键
    connect(events_, &SystemEvents::toggleRequested, this, [this]{
        if (shelf_) shelf_->toggleShelf();
    });
    connect(events_, &SystemEvents::captureRequested, this, &AppController::onCaptureShortcut);

    // 托盘
    connect(tray_, &TrayController::toggleRequested, this, [this]{
        if (shelf_) shelf_->toggleShelf();
    });
    connect(tray_, &TrayController::basketRequested, this, &AppController::onBasket);
    connect(tray_, &TrayController::textsRequested, this, &AppController::onTexts);
    connect(tray_, &TrayController::edgePopupChanged, this, [this](bool enabled){
        if (shelf_) shelf_->setEdgePopupEnabled(enabled);
        store_->state.edgePopup = enabled;
        store_->save();
        tray_->refresh();
        // 设置界面正开着时同步勾选状态，避免两边显示不一致
        if (settings_) settings_->syncFromState();
    });
    connect(tray_, &TrayController::collectClipboardChanged, this, [this](bool on){
        capture_->setCollectClipboard(on);
        if (settings_) settings_->syncFromState();
    });
    connect(tray_, &TrayController::collectTextClipboardChanged, this, [this](bool on){
        texts_->setCollectClipboard(on);
        if (settings_) settings_->syncFromState();
    });
    connect(tray_, &TrayController::autoHangChanged, this, [this](bool on){
        capture_->setAutoHang(on);
        if (settings_) settings_->syncFromState();
    });
    connect(tray_, &TrayController::settingsRequested, this, &AppController::onSettings);
    connect(tray_, &TrayController::captureRequested, this, &AppController::onCaptureShortcut);
    connect(tray_, &TrayController::openFolderRequested, this, [this]{
        QDir().mkpath(store_->inbox);
        QDesktopServices::openUrl(QUrl::fromLocalFile(store_->inbox));
    });
    connect(tray_, &TrayController::clearShelfRequested, this, [this]{
        gallery_->clear();   // 仅取下，保留文件
    });
    connect(tray_, &TrayController::exitRequested, this, [this]{
        stop();
        QCoreApplication::quit();
    });
}

void AppController::onClipboardChanged()
{
    capture_->clipboardChanged();
    if (!texts_->collectClipboard()) return;
    if (Clip::hasOwnFormat()) return;
    if (Clip::isImageContent()) return;      // 交给图片收集
    const QString text = Clip::readText();
    if (!text.trimmed().isEmpty()) texts_->add(text);
}

void AppController::onCopy(CaptureItem *item)
{
    QImage full(item->path);
    if (full.isNull() && !item->image.thumbnail.isNull())
        full = item->image.thumbnail.toImage();
    if (!full.isNull()) Clip::writeImage(item->path, full);
}

void AppController::onOpen(CaptureItem *item)
{
    // 内置查看器：同目录图片可左右切换；首次使用时创建，之后复用
    if (!viewer_) viewer_ = new ImageViewer(nullptr);
    viewer_->openFile(item->path);
}

void AppController::onEdit(CaptureItem *item)
{
    // 内置图片编辑器：后续迭代提供，当前先交给系统默认编辑器
    QDesktopServices::openUrl(QUrl::fromLocalFile(item->path));
}

void AppController::onTrash(CaptureItem *item)
{
    if (!item) return;
    const QString path = item->path;
    // 收件夹里的副本才移入回收站；外部截图只移除记录，保留原文件
    const bool ownCopy = QFileInfo(path).absoluteDir().absolutePath().compare(
        QDir(store_->inbox).absolutePath(), Qt::CaseInsensitive) == 0;
    gallery_->forget(item);
    if (ownCopy) QFile::moveToTrash(path);
}

void AppController::onSave(CaptureItem *item)
{
    const QString dest = QFileDialog::getSaveFileName(
        nullptr, tr("保存图片副本"),
        QFileInfo(item->path).fileName(),
        tr("图片 (*.png *.jpg *.jpeg *.bmp);;所有文件 (*)"));
    if (dest.isEmpty()) return;
    QFile::copy(item->path, dest);
}

void AppController::onLocate(CaptureItem *item)
{
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            { QStringLiteral("/select,"),
                              QDir::toNativeSeparators(QFileInfo(item->path).absoluteFilePath()) });
}

void AppController::onBasket()
{
    ensurePanels();
    basket_->refresh();
    showPanelBesideDock(basket_);
}

void AppController::onTexts()
{
    ensurePanels();
    textsWin_->refresh();
    showPanelBesideDock(textsWin_);
}

// 设置界面：仅从托盘右键菜单打开，显示在屏幕正中间
void AppController::onSettings()
{
    if (!settings_) {
        settings_ = new SettingsDialog(store_);
        connect(settings_, &SettingsDialog::themeSelected,
                this, &AppController::applyTheme);
        // 设置界面的顶部自动弹出与托盘菜单同一份状态，改动后同步两边
        connect(settings_, &SettingsDialog::edgePopupChanged, this, [this](bool enabled){
            if (shelf_) shelf_->setEdgePopupEnabled(enabled);
            store_->state.edgePopup = enabled;
            store_->save();
            tray_->refresh();
        });
        // 设置界面的收集开关 → 收集管线/文字库，同时刷新托盘勾选状态
        connect(settings_, &SettingsDialog::collectClipboardChanged, this, [this](bool on){
            capture_->setCollectClipboard(on);
            tray_->refresh();
        });
        connect(settings_, &SettingsDialog::collectTextClipboardChanged, this, [this](bool on){
            texts_->setCollectClipboard(on);
            tray_->refresh();
        });
        connect(settings_, &SettingsDialog::autoHangChanged, this, [this](bool on){
            capture_->setAutoHang(on);
            tray_->refresh();
        });
        // 卡片尺寸 / 不透明度 / 收起延迟：持久化并立即应用到挂绳
        connect(settings_, &SettingsDialog::cardSizeChanged, this, [this](int level){
            store_->state.cardSize = level;
            store_->save();
            if (shelf_) shelf_->setCardSize(level);
        });
        connect(settings_, &SettingsDialog::shelfOpacityChanged, this, [this](int pct){
            store_->state.shelfOpacity = pct;
            store_->save();
            if (shelf_) shelf_->setOpacityPercent(pct);
        });
        connect(settings_, &SettingsDialog::hideDelayChanged, this, [this](int ms){
            store_->state.hideDelayMs = ms;
            store_->save();
            if (shelf_) shelf_->setHideDelay(ms);
        });
        // 显示/收起挂绳的全局热键：重新注册；失败（多半被别的程序占用）则回滚
        connect(settings_, &SettingsDialog::toggleHotkeyChanged, this, [this](const QString &seq){
            if (events_->registerToggleHotkey(seq)) {
                activeHotkey_ = seq;
                return;
            }
            QMessageBox::warning(nullptr, tr("快捷键"),
                tr("快捷键 %1 注册失败，可能已被其它程序占用。").arg(seq));
            // 回滚到上一个可用热键，避免设置里显示成功、实际却唤不出挂绳
            store_->state.toggleHotkey = activeHotkey_.isEmpty()
                ? QStringLiteral("Ctrl+Alt+T") : activeHotkey_;
            store_->save();
            events_->registerToggleHotkey(store_->state.toggleHotkey);
            if (settings_) settings_->syncFromState();
        });
    }
    settings_->showCentered();
}

void AppController::applyTheme(const QString &id)
{
    if (!shelf_ || shelf_->theme().id == id) return;
    shelf_->setTheme(LineTheme::byId(id));
    shelf_->rebuild();
}

// 提前创建两个面板（隐藏状态），把窗口创建与列表构建的开销挪到启动后，
// 避免第一次打开面板时出现卡顿
void AppController::ensurePanels()
{
    if (!basket_) {
        basket_ = new BasketWindow(gallery_);
        connect(basket_, &BasketWindow::openRequested, this, [this](const QStringList &paths){
            if (paths.isEmpty()) return;
            if (!viewer_) viewer_ = new ImageViewer(nullptr);
            viewer_->openFile(paths.first());
        });
        connect(basket_, &BasketWindow::hangRequested, this, [this](const QStringList &paths){
            for (const QString &path : paths) {
                Gallery::ItemPtr ptr = gallery_->byPath(path);
                if (ptr) capture_->hang(ptr.data());
            }
        });
        connect(basket_, &BasketWindow::unhangRequested, this, [this](const QStringList &paths){
            QVector<CaptureItem *> targets;
            for (const QString &path : paths) {
                Gallery::ItemPtr ptr = gallery_->byPath(path);
                if (ptr) targets.append(ptr.data());
            }
            if (targets.isEmpty()) return;
            gallery_->removeMany(targets);   // 仅从绳子取下，保留收集记录
        });
        connect(basket_, &BasketWindow::deleteRequested, this, [this](const QStringList &paths){
            for (const QString &path : paths) {
                Gallery::ItemPtr ptr = gallery_->byPath(path);
                if (ptr) onTrash(ptr.data());
            }
        });
        connect(basket_, &BasketWindow::openFolderRequested, this, [this]{
            QDir().mkpath(store_->inbox);
            QDesktopServices::openUrl(QUrl::fromLocalFile(store_->inbox));
        });
        connect(basket_, &PanelWindow::hidden, this, &AppController::syncShelfHold);
        basket_->winId();          // 预创建原生窗口，消除首次显示的系统开销
    }
    if (!textsWin_) {
        textsWin_ = new TextsWindow(texts_);
        connect(textsWin_, &PanelWindow::hidden, this, &AppController::syncShelfHold);
        textsWin_->winId();
    }
}

void AppController::showPanelBesideDock(PanelWindow *panel)
{
    if (!panel) return;

    // 同时只显示一个面板：切换到另一个时先隐藏
    if (basket_ && basket_ != panel) basket_->hide();
    if (textsWin_ && textsWin_ != panel) textsWin_->hide();

    // 锚点取工具托板（三个按钮）；绳子收起时按屏幕右上角同一位置兜底
    QRect anchor;
    if (shelf_ && shelf_->isShownNow()) {
        anchor = shelf_->toolDockRect();
    } else {
        QScreen *s = QGuiApplication::screenAt(Win32::cursorPos());
        const QRect wa = s ? s->availableGeometry() : QRect(0, 0, 1280, 800);
        anchor = QRect(wa.right() - 72, wa.top() + 48, 40, 120);
    }
    panel->showPanel(anchor);
    syncShelfHold();
}

void AppController::syncShelfHold()
{
    if (!shelf_) return;
    const bool open = (basket_ && basket_->isVisible())
        || (textsWin_ && textsWin_->isVisible());
    shelf_->setHoldOpen(open);
}

void AppController::onCaptureShortcut()
{
    if (shelf_) shelf_->slideOut();
    // 调起系统截图工具（截图结果进入剪贴板，开启「剪贴板收集 · 图片」后自动挂绳）
    const QUrl uri(QStringLiteral("ms-screenclip:"));
    if (!QDesktopServices::openUrl(uri))
        QProcess::startDetached(QStringLiteral("explorer.exe"),
                                { QStringLiteral("ms-screenclip:") });
}
