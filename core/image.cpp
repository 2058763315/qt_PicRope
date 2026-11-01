#include "image.h"

#include <QFile>
#include <QBuffer>
#include <QCryptographicHash>
#include <QByteArrayView>

namespace {
const qint64 kMaxBytes = 64LL * 1024 * 1024;
const qint64 kMaxPixels = 80000000LL;

// 从 QImage 构造 ImageContent，并做尺寸校验
ImageContent build(const QImage &full, bool fingerprint)
{
    ImageContent c;
    if (full.isNull()) return c;
    if (full.width() <= 0 || full.height() <= 0
        || static_cast<qint64>(full.width()) * full.height() > kMaxPixels) return c;
    c.pixelWidth = full.width();
    c.pixelHeight = full.height();
    c.thumbnail = ImageContent::makeThumbnail(full);
    if (fingerprint) c.fingerprint = ImageContent::fingerprintOf(full);
    return c;
}
} // namespace

QPixmap ImageContent::makeThumbnail(const QImage &full)
{
    const int maxSide = 240;
    QImage scaled = (full.width() >= full.height())
        ? full.scaledToWidth(qMin(maxSide, full.width()), Qt::SmoothTransformation)
        : full.scaledToHeight(qMin(maxSide, full.height()), Qt::SmoothTransformation);
    // 转成 ARGB32 便于统一绘制
    return QPixmap::fromImage(scaled.convertToFormat(QImage::Format_ARGB32));
}

QByteArray ImageContent::fingerprintOf(const QImage &src)
{
    if (src.isNull()) return QByteArray();
    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    // 头部：宽高
    {
        const int w = img.width();
        const int h = img.height();
        QByteArray header;
        header.append(reinterpret_cast<const char *>(&w), sizeof(int));
        header.append(reinterpret_cast<const char *>(&h), sizeof(int));
        hash.addData(header);
    }
    // 采样：每 32 行取一行像素
    const int widthBytes = img.width() * 4;
    for (int y = 0; y < img.height(); y += 32) {
        const uchar *line = img.constScanLine(y);
        hash.addData(QByteArrayView(reinterpret_cast<const char *>(line), widthBytes));
    }
    return hash.result().toBase64();
}

ImageContent ImageContent::read(const QString &path, bool fingerprint)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return ImageContent();
    if (f.size() == 0 || f.size() > kMaxBytes) return ImageContent();
    QByteArray bytes = f.readAll();
    f.close();
    return decode(bytes, fingerprint);
}

ImageContent ImageContent::decode(const QByteArray &bytes, bool fingerprint)
{
    if (bytes.isEmpty() || bytes.size() > kMaxBytes) return ImageContent();
    QImage full;
    if (!full.loadFromData(bytes)) return ImageContent();
    return build(full, fingerprint);
}
