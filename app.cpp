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
#include "pinned.h"
#include "panels.h"
#include "tray.h"
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
    shelf_->slideIn(3500);

    // 启动后空闲时预热面板（图片篮子 / 文字信封），首次打开不再卡顿
    QTimer::singleShot(400, this, [this]{ ensurePanels(); });

    // 全局热键：等事件循环启动后再注册（更稳），并记录 Windows 错误码
    QTimer::singleShot(0, this, [this]{
        events_->registerToggleHotkey();
        if (!events_->toggleAvailable())
            store_->logError(QStringLiteral("Ctrl+Alt+T 热键注册失败（错误码 %1），"
                                            "可能已被其它程序占用")
                             .arg(events_->lastError()));
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
    for (const QPointer<PinnedImageWindow> &p : pins_)
        if (p) { p->hide(); p->deleteLater(); }
    pins_.clear();
    if (basket_) { basket_->hide(); basket_->deleteLater(); basket_ = nullptr; }
    if (textsWin_) { textsWin_->hide(); textsWin_->deleteLater(); textsWin_ = nullptr; }
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
    connect(shelf_, &ShelfWindow::detachRequested, this, &AppController::onDetach);
    connect(shelf_, &ShelfWindow::trashRequested, this, &AppController::onTrash);
    connect(shelf_, &ShelfWindow::saveRequested, this, &AppController::onSave);
    connect(shelf_, &ShelfWindow::locateRequested, this, &AppController::onLocate);
    connect(shelf_, &ShelfWindow::basketRequested, this, &AppController::onBasket);
    connect(shelf_, &ShelfWindow::textsRequested, this, &AppController::onTexts);
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
    connect(tray_, &TrayController::collectClipboardChanged, this, [this](bool on){
        capture_->setCollectClipboard(on);
    });
    connect(tray_, &TrayController::collectTextClipboardChanged, this, [this](bool on){
        texts_->setCollectClipboard(on);
    });
    connect(tray_, &TrayController::autoHangChanged, this, [this](bool on){
        capture_->setAutoHang(on);
    });
    connect(tray_, &TrayController::themeSelected, this, [this](const QString &id){
        if (store_->state.theme == id) return;
        store_->state.theme = id;
        store_->save();
        if (shelf_) {
            shelf_->setTheme(LineTheme::byId(id));
            shelf_->rebuild();
        }
        tray_->refresh();
    });
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
    QDesktopServices::openUrl(QUrl::fromLocalFile(item->path));
}

void AppController::onEdit(CaptureItem *item)
{
    // 内置图片编辑器：后续迭代提供，当前先交给系统默认编辑器
    QDesktopServices::openUrl(QUrl::fromLocalFile(item->path));
}

void AppController::onDetach(CaptureItem *item)
{
    Gallery::ItemPtr ptr = gallery_->ptrOf(item);
    if (!ptr) return;

    // 取下后从绳上移除，关闭贴图时再挂回
    gallery_->remove(item);

    auto *pin = new PinnedImageWindow(ptr, nullptr);
    pins_.append(pin);

    connect(pin, &PinnedImageWindow::closed, this, [this, pin]{
        Gallery::ItemPtr p = pin->itemPtr();
        if (p) gallery_->add(p->path, p->image);
        pin->hide();
        pin->deleteLater();
        pins_.removeAll(pin);
        if (shelf_) shelf_->slideIn(2000);
    });
    connect(pin, &PinnedImageWindow::removeRequested, this, [this, pin]{
        if (CaptureItem *it = pin->item()) onTrash(it);
        pin->hide();
        pin->deleteLater();
        pins_.removeAll(pin);
    });

    pin->placeAtScreen(Win32::screenAt(Win32::cursorPos()).workArea);
    pin->show();
    pin->raise();
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
        nullptr, QStringLiteral("保存图片副本"),
        QFileInfo(item->path).fileName(),
        QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp);;所有文件 (*)"));
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

// 提前创建两个面板（隐藏状态），把窗口创建与列表构建的开销挪到启动后，
// 避免第一次打开面板时出现卡顿
void AppController::ensurePanels()
{
    if (!basket_) {
        basket_ = new BasketWindow(gallery_);
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
