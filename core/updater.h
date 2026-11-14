#pragma once
// =====================================================================
//  core/updater.h — 检查与下载 GitHub 最新发行版
//  API: https://api.github.com/repos/2058763315/qt_PicRope/releases/latest
// =====================================================================

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;
class QWidget;

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker() override;

    // a > b 返回正数，相等 0，a < b 返回负数（忽略前导 v，按数字段比较）
    static int compareVersions(const QString &a, const QString &b);
    static QString currentVersion();

    void checkLatest();
    void download(const QUrl &url);

signals:
    void upToDate(const QString &version);
    void updateAvailable(const QString &version, const QUrl &setupUrl, const QUrl &pageUrl);
    void downloadProgress(int percent);
    void downloadFinished(const QString &filePath);
    void failed(const QString &message);

private:
    void onCheckFinished();
    void onDownloadReadyRead();
    void onDownloadFinished();

    QNetworkAccessManager *net_;
    QNetworkReply *checkReply_ = nullptr;
    QNetworkReply *dlReply_ = nullptr;
    QFile *dlFile_ = nullptr;
    QString dlPath_;
    QString latestVersion_;
};
