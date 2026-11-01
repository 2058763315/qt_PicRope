#pragma once
// =====================================================================
//  core/image.h — 图片内容（对应原 PicRope 的 ImageContent）
//  负责读取校验、缩略图生成与像素指纹（去重用）
// =====================================================================

#include <QString>
#include <QImage>
#include <QPixmap>
#include <QByteArray>

// 一张收藏图片的展示内容
class ImageContent
{
public:
    ImageContent() = default;

    int pixelWidth = 0;
    int pixelHeight = 0;
    QPixmap thumbnail;       // 缩略图（最长边 ≤ 240）
    QByteArray fingerprint;  // 像素指纹（SHA-256 采样，base64）

    bool valid() const { return pixelWidth > 0 && pixelHeight > 0; }

    // 从文件读取（带校验：大小 ≤ 64MB，尺寸 ≤ 8000 万像素）
    static ImageContent read(const QString &path, bool computeFingerprint = true);

    // 从字节读取
    static ImageContent decode(const QByteArray &bytes, bool computeFingerprint = true);

    // 对 QImage 生成指纹
    static QByteArray fingerprintOf(const QImage &img);

    // 生成缩略图
    static QPixmap makeThumbnail(const QImage &full);
};
