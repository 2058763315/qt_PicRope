#include "clipboard.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QBuffer>
#include <QFileInfo>
#include <QSet>
#include <QVariant>

namespace {

const char *kOwnFormat = "PicRope.OwnClipboard";

bool imageKind(const QMimeData *m)
{
    if (!m) return false;
    if (m->hasImage()) return true;
    const QStringList formats = m->formats();
    for (const QString &f : formats)
        if (f.startsWith("image/")) return true;
    return false;
}

// 是否混入文本 / 文件 / HTML 等非纯图片格式
bool mixedWithText(const QMimeData *m)
{
    const QStringList formats = m->formats();
    static const QSet<QString> excluded = {
        "text/plain", "text/html", "text/uri-list", "application/x-qt-image",
        "text/richtext", "application/rtf", "text/rtf"
    };
    for (const QString &f : formats)
        if (excluded.contains(f)) return true;
    return false;
}

bool isImageFile(const QString &path)
{
    static const QSet<QString> exts = { ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tif", ".tiff" };
    return exts.contains(QFileInfo(path).suffix().toLower());
}

} // namespace

namespace Clip {

bool hasOwnFormat()
{
    const QMimeData *m = QGuiApplication::clipboard()->mimeData();
    return m && m->hasFormat(QLatin1String(kOwnFormat));
}

bool isImageContent()
{
    if (hasOwnFormat()) return false;      // 自己写入的回显，跳过
    const QMimeData *m = QGuiApplication::clipboard()->mimeData();
    if (!m) return false;
    if (!imageKind(m)) return false;
    // 纯图片：没有文本/HTML/文件混入
    if (!mixedWithText(m)) return true;
    // 微信式：单个图片文件 + 图片数据、无纯文本
    const QString f = imageFileFromUrls();
    return !f.isEmpty() && !m->hasText();
}

QString imageFileFromUrls()
{
    const QMimeData *m = QGuiApplication::clipboard()->mimeData();
    if (!m || !m->hasUrls()) return QString();
    const QList<QUrl> urls = m->urls();
    if (urls.size() != 1 || !urls[0].isLocalFile()) return QString();
    const QString path = urls[0].toLocalFile();
    return isImageFile(path) ? path : QString();
}

QImage readImage()
{
    const QMimeData *m = QGuiApplication::clipboard()->mimeData();
    if (!m) return QImage();
    if (m->hasImage())
        return qvariant_cast<QImage>(m->imageData());
    return QImage();
}

QString readText()
{
    const QMimeData *m = QGuiApplication::clipboard()->mimeData();
    if (!m || imageKind(m)) return QString();   // 含图片时不算纯文本
    return m->text();
}

void writeImage(const QString &filePath, const QImage &img)
{
    if (img.isNull()) return;
    auto *m = new QMimeData;
    m->setImageData(img);
    // PNG 原始字节
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    m->setData(QStringLiteral("PNG"), png);
    // 文件引用（便于拖入目标应用）
    if (!filePath.isEmpty())
        m->setUrls({ QUrl::fromLocalFile(filePath) });
    m->setData(QLatin1String(kOwnFormat), QByteArrayLiteral("1"));
    QGuiApplication::clipboard()->setMimeData(m, QClipboard::Clipboard);
}

void resetOwnState()
{
    // 暂无状态，保留占位
}

} // namespace Clip
