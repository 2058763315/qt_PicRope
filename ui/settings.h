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
class QLabel;
class QProgressBar;
class QPushButton;
class QVBoxLayout;

class SettingsDialog : public PanelWindow
{
    Q_OBJECT
public:
    explicit SettingsDialog(StateStore *store, QWidget *parent = nullptr);

    void showCentered();

signals:
    void themeSelected(const QString &id);

private:
    void buildAppearance(QVBoxLayout *root);
    void buildAbout(QVBoxLayout *root);
    void startManualCheck();
    void beginDownload(const QUrl &url);
    void launchInstaller(const QString &filePath);
    void resetUpdateUi();

    StateStore *store_;
    QButtonGroup *themeGroup_ = nullptr;
    QPushButton *checkBtn_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *percent_ = nullptr;
    QLabel *status_ = nullptr;
    UpdateChecker *updater_ = nullptr;
    bool downloading_ = false;
};
