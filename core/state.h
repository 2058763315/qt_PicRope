#pragma once
// =====================================================================
//  core/state.h — 状态持久化（对应原 PicRope 的 SavedState + StateStore）
//  设置与收藏路径保存在程序旁 data/ 目录（state.json）
// =====================================================================

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVector>

// 一条文字收藏
struct TextItem {
    QString id;
    QString content;
    QDateTime createdUtc;
    bool    marked = false;   // 是否被勾选（仅内存态，不持久化）
};

// 全部可持久化状态
struct SavedState {
    bool   collectClipboard      = true;
    bool   collectTextClipboard  = true;
    bool   autoHang              = true;
    bool   autoCheckUpdate       = true;   // 设置界面：自动检查更新
    bool   edgePopup             = true;   // 鼠标移到屏幕顶部自动弹出挂绳
    QString language             = QStringLiteral("zh_CN");   // 界面语言
    bool   hoverPreview          = true;
    QString theme                = QStringLiteral("classic");
    int    cardSize               = 1;     // 卡片尺寸：0 小 / 1 中 / 2 大
    int    shelfOpacity           = 100;   // 挂绳不透明度（20~100%）
    int    hideDelayMs            = 1200;  // 鼠标离开后自动收起延迟（毫秒）
    QString toggleHotkey          = QStringLiteral("Ctrl+Alt+T");  // 显示/收起挂绳的全局热键
    QString displayDevice;                 // 空 = 跟随鼠标
    QString lastUpdateNotification;
    unsigned captureModifiers = 3;         // Ctrl+Alt
    unsigned captureKey        = 'S';
    QStringList images;                    // 挂绳的图片路径（最近 50）
    QStringList collection;                // 已收集的全部图片路径
    QVector<TextItem> texts;
};

class QJsonObject;

class StateStore
{
public:
    // root = 程序目录下 data
    explicit StateStore(const QString &root);

    const QString root;      // data 目录
    const QString inbox;     // data/Captures
    SavedState state;

    bool load();             // 读取 state.json；不存在则用默认值
    bool save();             // 原子写入 state.json
    void logError(const QString &message);

private:
    QString file_;
};

// 解析 / 序列化（供内部使用）
namespace StateJson {
QJsonObject toJson(const SavedState &s);
bool        fromJson(const QJsonObject &o, SavedState &s);
}
