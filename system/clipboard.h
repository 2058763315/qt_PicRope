#pragma once
// =====================================================================
//  system/clipboard.h — 剪贴板读写（图片 / 文本 / 文件引用）
//  对应原 PicRope 的 WindowsClipboard.cs + ClipboardRules.cs
// =====================================================================

#include <QString>
#include <QImage>

namespace Clip {

// 是否包含“图绳自己写入”的标记（用于跳过自身写入的回显）
bool hasOwnFormat();

// 剪贴板是否应作为“图片”收集（纯图片，或单个图片文件）
bool isImageContent();
// 若剪贴板含单个图片文件，返回其路径；否则空
QString imageFileFromUrls();

// 读取图片（若存在），失败返回空 QImage
QImage readImage();

// 读取纯文本（若剪贴板不含图片/文件），否则返回空
QString readText();

// 写入：图片 + 文件引用 + PNG 字节 + 自有标记（用于“复制图片”）
void writeImage(const QString &filePath, const QImage &img);

// 清空剪贴板上的自有标记
void resetOwnState();

}
