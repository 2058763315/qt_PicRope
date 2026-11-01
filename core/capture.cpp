#include "capture.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>
#include <QBuffer>
#include <QGuiApplication>
#include <algorithm>

#include "clipboard.h"

namespace {
bool isImageFile(const QString &path)
{
    static const QStringList exts = { ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tif", ".tiff" };
    return exts.contains(QFileInfo(path).suffix().toLower());
}
enum Source { SourceFolder = 0, SourceClipboard = 1 };
}

CapturePipeline::CapturePipeline(Gallery *gallery, const QString &folder, QObject *parent)
    : QObject(parent), gallery_(gallery), folder_(folder)
{
    connect(&timer_, &QTimer::timeout, this, &CapturePipeline::tick);
    timer_.setInterval(200);
    connect(&clipboardTimer_, &QTimer::timeout, this, &CapturePipeline::readClipboardImage);
    clipboardTimer_.setSingleShot(true);
    clipboardTimer_.setInterval(160);
}

void CapturePipeline::start()
{
    // 首次扫描现有文件：早于启动时间的视为已存在，不收集
    const qint64 launched = QDateTime::currentMSecsSinceEpoch();
    QDir dir(folder_);
    if (dir.exists()) {
        const auto files = dir.entryList(QDir::Files, QDir::Time);
        for (const QString &name : files) {
            QString p = dir.absoluteFilePath(name);
            if (!isImageFile(p)) continue;
            lastFiles_.insert(p);
            if (QFileInfo(p).lastModified().toMSecsSinceEpoch() < launched)
                seen_.insert(p);
        }
    }
    // 已保存的收集已由 Gallery 在构造时恢复，这里只补扫收件夹中遗漏的副本
    // 扫描 inbox 中遗漏的
    QDir inbox(store()->inbox);
    if (inbox.exists()) {
        const auto files = inbox.entryList(QDir::Files, QDir::Time);
        for (const QString &name : files) {
            QString p = inbox.absoluteFilePath(name);
            if (!isImageFile(p) || gallery_->containsInCollected(p)) continue;
            ImageContent img = ImageContent::read(p, false);
            if (img.valid()) gallery_->collect(p, img);
        }
    }
    timer_.start();
}

void CapturePipeline::setCollectClipboard(bool on)
{
    if (store()->state.collectClipboard == on) return;
    store()->state.collectClipboard = on;
    store()->save();
    clipDueMs_ = -1;
    gallery_->notifyChanged();
}

void CapturePipeline::setAutoHang(bool on)
{
    if (store()->state.autoHang == on) return;
    store()->state.autoHang = on;
    store()->save();
    gallery_->notifyChanged();
}

void CapturePipeline::ignoreCurrentClipboard()
{
    // 记录当前剪贴板 "内容签名"，下次变化时跳过——用序列号近似
    // Qt 无直接 API，改用一个布尔：忽略紧接的第一次 dataChanged
    clipJustIgnored_ = true;
    clipDueMs_ = -1;
}

void CapturePipeline::tick()
{
    scanFolder();
    processPending();
    refreshDirty();
    // 剪贴板延迟读取
    if (!clipBusy_ && clipDueMs_ >= 0 && QDateTime::currentMSecsSinceEpoch() >= clipDueMs_) {
        clipDueMs_ = -1;
        readClipboardImage();
    }
}

void CapturePipeline::scanFolder()
{
    QDir dir(folder_);
    if (!dir.exists()) return;
    const auto files = dir.entryList(QDir::Files, QDir::Time);
    QSet<QString> current;
    for (const QString &name : files) {
        QString p = dir.absoluteFilePath(name);
        if (!isImageFile(p)) continue;
        current.insert(p);
        if (!seen_.contains(p) && !pending_.contains(p) && !processing_.contains(p))
            pending_.insert(p, 0);
    }
    lastFiles_ = current;
}

void CapturePipeline::processPending()
{
    const auto keys = pending_.keys();
    for (const QString &path : keys) {
        if (processing_.contains(path)) continue;
        processing_.insert(path);
        bool complete = import(path, true, true);
        processing_.remove(path);
        if (complete) { pending_.remove(path); seen_.insert(path); }
        else if (pending_.contains(path)) {
            if (++pending_[path] >= 20) pending_.remove(path);
        }
    }
}

void CapturePipeline::refreshDirty()
{
    // 已收集且内容被外部改写的图片，周期性刷新（简化：每 2 秒检查一次）
    static int counter = 0;
    if (++counter % 10 != 0) return;
    bool any = false;
    for (const Gallery::ItemPtr &ptr : gallery_->collected) {
        if (processing_.contains(ptr->path)) continue;
        if (QFileInfo(ptr->path).lastModified() != ptr->written) {
            ImageContent img = ImageContent::read(ptr->path, false);
            if (img.valid()) {
                ptr->image = img;
                ptr->written = QFileInfo(ptr->path).lastModified();
                any = true;
            }
        }
    }
    if (any) gallery_->notifyChanged();
}

bool CapturePipeline::receiptsAccept(const QByteArray &hash, int source)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    recent_.erase(std::remove_if(recent_.begin(), recent_.end(),
        [now](const Receipt &r) { return now - r.timeMs > 5000; }), recent_.end());
    for (Receipt &r : recent_) {
        if (!r.paired && r.hash == hash && r.source != source) { r.paired = true; return false; }
    }
    Receipt r; r.hash = hash; r.source = source; r.timeMs = now; r.paired = false;
    recent_.append(r);
    return true;
}

bool CapturePipeline::import(const QString &path, bool fromSource, bool announce)
{
    if (fromSource ? gallery_->containsInCollected(path) : gallery_->contains(path))
        return true;

    ImageContent img = ImageContent::read(path);
    if (!img.valid()) return false;

    if (fromSource && !receiptsAccept(img.fingerprint, SourceFolder)) return true;

    // 凡不在收件夹内的图片，一律复制进收件夹管理
    QString use = path;
    const QString inboxPath = QDir(store()->inbox).absolutePath();
    if (!QFileInfo(path).absoluteFilePath().startsWith(inboxPath, Qt::CaseInsensitive)) {
        const QString copy = copyToInbox(path);
        if (copy.isEmpty()) return false;
        use = copy;
        img = ImageContent::read(use);
        if (!img.valid()) return false;
    }

    if (!fromSource || !autoHang()) gallery_->collect(use, img);
    else {
        gallery_->add(use, img);
        if (announce) emit arrived(use, fromSource);
    }
    return true;
}

QString CapturePipeline::copyToInbox(const QString &path)
{
    QDir().mkpath(store()->inbox);
    QFileInfo info(path);
    const QString suffix = info.suffix();
    QString name = info.completeBaseName() + "_" +
        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)
        + (suffix.isEmpty() ? QStringLiteral(".png") : QStringLiteral(".") + suffix);
    QString dest = store()->inbox + "/" + name;
    if (QFile::copy(path, dest)) return dest;
    return QString();
}

void CapturePipeline::hang(CaptureItem *item)
{
    if (gallery_->contains(item->path)) return;
    if (!QFileInfo::exists(item->path)) { gallery_->prune(); return; }
    gallery_->add(item->path, item->image);
    emit arrived(item->path, false);
}

QString CapturePipeline::acceptCapture(const QImage &picture)
{
    if (picture.isNull()) return QString();
    QByteArray png;
    {
        QBuffer buf(&png);
        buf.open(QIODevice::WriteOnly);
        picture.save(&buf, "PNG");
    }
    ImageContent img = ImageContent::decode(png, true);
    QDir().mkpath(store()->inbox);
    QString path = store()->inbox + "/截图_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz") + "_" +
        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8) + ".png";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return QString();
    f.write(png);
    f.close();

    receiptsAccept(img.fingerprint, SourceClipboard);
    if (autoHang()) { gallery_->add(path, img); emit arrived(path, false); }
    else gallery_->collect(path, img);

    // 写回剪贴板（含自有标记，避免再次收集自己）
    Clip::writeImage(path, picture);
    ignoreCurrentClipboard();
    return path;
}

void CapturePipeline::clipboardChanged()
{
    if (!collectClipboard()) return;
    if (clipJustIgnored_) { clipJustIgnored_ = false; return; }
    clipDueMs_ = QDateTime::currentMSecsSinceEpoch() + 160;
}

void CapturePipeline::readClipboardImage()
{
    if (!collectClipboard() || clipBusy_) return;
    clipBusy_ = true;

    QByteArray bytes;
    QString ext = ".png";
    ImageContent content;
    const QString filePath = Clip::imageFileFromUrls();
    const QImage img = Clip::readImage();

    if (!filePath.isEmpty() && img.isNull()) {
        // 剪贴板携带单个图片文件引用（如微信/文件管理器）
        content = ImageContent::read(filePath, true);
        if (!content.valid()) { clipBusy_ = false; return; }
        QFile src(filePath);
        if (src.open(QIODevice::ReadOnly)) bytes = src.readAll();
        if (bytes.isEmpty() || bytes.size() > 64LL * 1024 * 1024) { clipBusy_ = false; return; }
        ext = QFileInfo(filePath).suffix().toLower();
        if (ext.isEmpty()) ext = "png";
    } else if (!img.isNull()) {
        QBuffer buf(&bytes);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");
        content = ImageContent::decode(bytes, true);
    } else {
        clipBusy_ = false;
        return;
    }

    if (!content.valid() || !receiptsAccept(content.fingerprint, SourceClipboard)) { clipBusy_ = false; return; }

    QDir().mkpath(store()->inbox);
    QString path = store()->inbox + "/截图_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz") + "_" +
        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8) + "." + ext;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { clipBusy_ = false; return; }
    f.write(bytes);
    f.close();

    if (!autoHang()) gallery_->collect(path, content);
    else {
        gallery_->add(path, content);
        emit arrived(path, true);
    }
    clipBusy_ = false;
}
