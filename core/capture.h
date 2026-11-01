#pragma once
// =====================================================================
//  core/capture.h — 截图收集管线（对应原 PicRope 的 CapturePipeline）
//  负责：截图文件夹新图片收集、剪贴板图片收集、去重、收进篮子或挂绳
// =====================================================================

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <QList>
#include <QByteArray>

#include "gallery.h"

class CapturePipeline : public QObject
{
    Q_OBJECT
public:
    explicit CapturePipeline(Gallery *gallery, const QString &folder, QObject *parent = nullptr);

    void start();

    bool collectClipboard() const { return store()->state.collectClipboard; }
    void setCollectClipboard(bool on);

    bool autoHang() const { return store()->state.autoHang; }
    void setAutoHang(bool on);

    void ignoreCurrentClipboard();     // 跳过当前剪贴板内容

    // 手动导入一张图片（source=false 表示手动添加，非系统截图来源）
    bool import(const QString &path, bool fromSource, bool announce);
    QString copyToInbox(const QString &path);
    void hang(CaptureItem *item);
    // 接受一次区域截图并落盘，返回保存路径
    QString acceptCapture(const QImage &picture);

    // 剪贴板内容变化（由 QClipboard::dataChanged 触发）
    void clipboardChanged();

signals:
    void arrived(const QString &path, bool fromClipboard);
    void failed(const QString &message);

private:
    StateStore *store() const { return gallery_->store(); }

    void tick();
    void scanFolder();
    void processPending();
    void readClipboardImage();
    void refreshDirty();

    bool receiptsAccept(const QByteArray &hash, int source);

    Gallery *gallery_;
    QString folder_;
    QTimer timer_;
    QTimer clipboardTimer_;

    QSet<QString> seen_;            // 已处理的文件夹文件
    QHash<QString, int> pending_;   // 待处理文件及重试计数
    QSet<QString> processing_;
    QSet<QString> lastFiles_;       // 上次扫描到的文件集合

    quint32 clipSequence_ = 0;      // 剪贴板序列号快照（用时间戳近似）
    qint64 clipDueMs_ = -1;
    bool clipBusy_ = false;
    bool clipJustIgnored_ = false;

    struct Receipt { QByteArray hash; int source; qint64 timeMs; bool paired; };
    QList<Receipt> recent_;
};
