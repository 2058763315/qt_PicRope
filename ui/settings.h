#pragma once
// =====================================================================
//  ui/settings.h — 设置界面（目前仅含外观与关于）
//  只从托盘右键菜单打开，窗口显示在屏幕正中间
// =====================================================================

#include <QUrl>

#include "panels.h"

class StateStore;
class UpdateChecker;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QKeySequenceEdit;
class QProgressBar;
class QPushButton;
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

private:
    void buildFeatures(QVBoxLayout *root);
    void buildShortcuts(QVBoxLayout *root);
    void buildAppearance(QVBoxLayout *root);
    void buildData(QVBoxLayout *root);
    void buildAbout(QVBoxLayout *root);
    void startManualCheck();
    void beginDownload(const QUrl &url);
    void launchInstaller(const QString &filePath);
    void resetUpdateUi();

    StateStore *store_;
    QButtonGroup *themeGroup_ = nullptr;
    QCheckBox *edgeCheck_ = nullptr;
    QCheckBox *startCheck_ = nullptr;
    QCheckBox *clipCheck_ = nullptr;     // 剪贴板收集 · 图片
    QCheckBox *textCheck_ = nullptr;     // 剪贴板收集 · 文字
    QCheckBox *hangCheck_ = nullptr;     // 新图片自动上绳
    QComboBox *sizeCombo_ = nullptr;     // 卡片尺寸
    QComboBox *opacityCombo_ = nullptr;  // 挂绳不透明度
    QComboBox *delayCombo_ = nullptr;    // 自动收起延迟
    QKeySequenceEdit *hotkeyEdit_ = nullptr;  // 显示/收起挂绳热键
    QPushButton *checkBtn_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *percent_ = nullptr;
    QLabel *status_ = nullptr;
    QHBoxLayout *updateRow_ = nullptr;   // 版本号所在行，用于切换弹簧/进度条的拉伸
    int updateSpacerIndex_ = -1;         // 行内弹簧的索引
    UpdateChecker *updater_ = nullptr;
    bool downloading_ = false;
};
