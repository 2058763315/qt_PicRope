#pragma once
// =====================================================================
//  ui/settings.h — 设置界面（目前仅含外观与关于）
//  只从托盘右键菜单打开，窗口显示在屏幕正中间
// =====================================================================

#include <QUrl>
#include <QHash>

#include "panels.h"

class StateStore;
class UpdateChecker;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QKeySequenceEdit;
class QLineEdit;
class QProgressBar;
class QProcess;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QTimer;
class QVBoxLayout;

class SettingsDialog : public PanelWindow
{
    Q_OBJECT
public:
    explicit SettingsDialog(StateStore *store, QWidget *parent = nullptr);

    void showCentered();
    void syncFromState();          // 按最新状态刷新控件（托盘可能在设置打开时改动过）

signals:
    void themeSelected(const QString &id);
    void edgePopupChanged(bool enabled);   // 鼠标到屏幕顶部自动弹出挂绳
    void collectClipboardChanged(bool on); // 剪贴板收集 · 图片
    void collectTextClipboardChanged(bool on); // 剪贴板收集 · 文字
    void autoHangChanged(bool on);         // 新图片自动上绳
    void cardSizeChanged(int level);       // 卡片尺寸：0 小 / 1 中 / 2 大
    void shelfOpacityChanged(int pct);     // 挂绳不透明度（20~100）
    void hideDelayChanged(int ms);         // 自动收起延迟
    void toggleHotkeyChanged(const QString &seq);  // 显示/收起挂绳的全局热键
    void cleanupRequested();                       // 自动清理：用户确认立即按上限裁剪

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;   // 可修改快捷键：点击聚焦后提示“请按下组合键”

private:
    void buildFeatures(QVBoxLayout *root);
    QVBoxLayout *addSectionCard(QVBoxLayout *root);   // 设置分区：带底色圆角容器，内部统一行距
    QWidget *buildNav();                              // 左侧分区导航
    void scrollToSection(const QString &key);         // 点击导航跳转到对应分区
    void buildShortcuts(QVBoxLayout *root);
    void applyHotkeyPlaceholder();   // 使用独立标签居中显示等待提示，避免 Qt 占位提示限制
    void applyLimitChange(bool images);   // 保存上限；低于当前数量时询问是否立即清理
    void buildAppearance(QVBoxLayout *root);
    void buildData(QVBoxLayout *root);
    void buildOther(QVBoxLayout *root);
    void buildAbout(QVBoxLayout *root);
    void startManualCheck();
    void beginDownload(const QUrl &url);
    void launchInstaller(const QString &filePath);
    void resetUpdateUi();

    StateStore *store_;
    QScrollArea *scroll_ = nullptr;          // 右侧内容滚动区
    QButtonGroup *navGroup_ = nullptr;       // 左侧分区导航按钮
    QHash<QString, QWidget *> anchors_;      // 分区 key → 标题控件，用于跳转与高亮
    bool navPinning_ = false;                // 点击导航后的高亮锁定期
    QButtonGroup *themeGroup_ = nullptr;
    QCheckBox *edgeCheck_ = nullptr;
    QCheckBox *startCheck_ = nullptr;
    QCheckBox *clipCheck_ = nullptr;     // 剪贴板收集 · 图片
    QCheckBox *textCheck_ = nullptr;     // 剪贴板收集 · 文字
    QCheckBox *hangCheck_ = nullptr;     // 新图片自动上绳
    QCheckBox *cleanCheck_ = nullptr;    // 自动清理
    QWidget *limitBox_ = nullptr;        // 上限设置容器（勾选自动清理后显示）
    QSpinBox *imageLimitSpin_ = nullptr; // 图片上限（挂绳 + 篮子合计）
    QSpinBox *textLimitSpin_ = nullptr;  // 文字笔记上限
    bool limitPrompting_ = false;        // 上限确认弹窗进行中，防止重复弹出
    QComboBox *sizeCombo_ = nullptr;     // 卡片尺寸
    QComboBox *opacityCombo_ = nullptr;  // 挂绳不透明度
    QComboBox *delayCombo_ = nullptr;    // 自动收起延迟
    QKeySequenceEdit *hotkeyEdit_ = nullptr;  // 显示/收起挂绳热键
    QLineEdit *hotkeyLine_ = nullptr;         // hotkeyEdit_ 内部的输入框，用于显示按键
    QLabel *hotkeyHint_ = nullptr;            // 独立居中的“请按下快捷键”提示层
    QPushButton *checkBtn_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *percent_ = nullptr;
    QLabel *status_ = nullptr;
    QHBoxLayout *updateRow_ = nullptr;   // 版本号所在行，用于切换弹簧/进度条的拉伸
    int updateSpacerIndex_ = -1;         // 行内弹簧的索引
    UpdateChecker *updater_ = nullptr;
    bool downloading_ = false;

    // 数据导出（异步 + 进度）
    QPushButton *exportBtn_ = nullptr;
    QProgressBar *exportProgress_ = nullptr;
    QLabel *exportPercent_ = nullptr;
    QLabel *exportStatus_ = nullptr;
    QHBoxLayout *exportRow_ = nullptr;
    int exportSpacerIndex_ = -1;
    QProcess *exportProc_ = nullptr;
    QTimer *exportTimer_ = nullptr;
    QString exportPath_;
    qint64 exportTotal_ = 0;
};
