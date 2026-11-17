#include "updater.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QVector>

namespace {
const char *kApiUrl =
    "https://api.github.com/repos/2058763315/qt_PicRope/releases/latest";
const char *kUserAgent = "PicRope-Updater";
}

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), net_(new QNetworkAccessManager(this))
{
}

UpdateChecker::~UpdateChecker()
{
    if (checkReply_) checkReply_->abort();
    if (dlReply_) dlReply_->abort();
    if (dlFile_) { dlFile_->close(); dlFile_->deleteLater(); }
}

QString UpdateChecker::currentVersion()
{
    const QString v = QCoreApplication::applicationVersion();
    return v.isEmpty() ? QStringLiteral("0.0.0") : v;
}

int UpdateChecker::compareVersions(const QString &a, const QString &b)
{
    auto parts = [](QString v) {
        QVector<int> out;
        v = v.trimmed();
        if (v.startsWith('v') || v.startsWith('V')) v = v.mid(1);
        const QStringList segs = v.split('.');
        for (const QString &s : segs) {
            int n = 0;
            bool any = false;
            for (const QChar c : s) {
                if (!c.isDigit()) break;
                n = n * 10 + c.digitValue();
                any = true;
            }
            out.append(any ? n : 0);
        }
        return out;
    };

    const QVector<int> pa = parts(a);
    const QVector<int> pb = parts(b);
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int x = i < pa.size() ? pa.at(i) : 0;
        const int y = i < pb.size() ? pb.at(i) : 0;
        if (x != y) return x - y;
    }
    return 0;
}

void UpdateChecker::checkLatest()
{
    if (checkReply_) return;

    QNetworkRequest req(QUrl(QString::fromLatin1(kApiUrl)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    req.setRawHeader("Accept", "application/vnd.github+json");

    checkReply_ = net_->get(req);
    connect(checkReply_, &QNetworkReply::finished, this, &UpdateChecker::onCheckFinished);
}

void UpdateChecker::onCheckFinished()
{
    QNetworkReply *reply = checkReply_;
    checkReply_ = nullptr;
    if (!reply) return;
    reply->deleteLater();

    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit failed(tr("获取更新信息失败（%1%2）")
                        .arg(reply->errorString(),
                             status > 0 ? tr("，HTTP %1").arg(status) : QString()));
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit failed(tr("更新信息解析失败：") + err.errorString());
        return;
    }

    const QJsonObject rel = doc.object();
    const QString tag = rel.value("tag_name").toString();
    if (tag.isEmpty()) {
        emit failed(tr("最新发行版缺少版本号"));
        return;
    }
    latestVersion_ = tag;
    const QString pageUrl = rel.value("html_url").toString();

    // 优先取名字里带 Setup 的 exe，其次任意 exe
    QUrl setupUrl;
    QUrl anyExe;
    const QJsonArray assets = rel.value("assets").toArray();
    for (const QJsonValue &v : assets) {
        const QJsonObject a = v.toObject();
        const QString name = a.value("name").toString();
        const QString url = a.value("browser_download_url").toString();
        if (url.isEmpty()) continue;
        if (name.endsWith(".exe", Qt::CaseInsensitive)) {
            if (anyExe.isEmpty()) anyExe = QUrl(url);
            if (name.contains("setup", Qt::CaseInsensitive)) { setupUrl = QUrl(url); break; }
        }
    }
    if (setupUrl.isEmpty()) setupUrl = anyExe;

    // 【临时改为颠倒判断，用于模拟更新下载；验证后请恢复为 > 0】
    if (compareVersions(tag, currentVersion()) > 0)
        emit updateAvailable(tag, setupUrl, QUrl(pageUrl));
    else
        emit upToDate(currentVersion());
}

void UpdateChecker::download(const QUrl &url)
{
    if (dlReply_ || url.isEmpty()) {
        emit failed(tr("没有可下载的安装包"));
        return;
    }

    QString name = url.fileName();
    if (name.isEmpty()) name = QStringLiteral("PicRope-Setup.exe");
    dlPath_ = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + QLatin1Char('/') + name;

    dlFile_ = new QFile(dlPath_, this);
    if (!dlFile_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit failed(tr("无法写入临时文件：") + dlPath_);
        dlFile_->deleteLater();
        dlFile_ = nullptr;
        return;
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    dlReply_ = net_->get(req);
    connect(dlReply_, &QNetworkReply::readyRead, this, &UpdateChecker::onDownloadReadyRead);
    connect(dlReply_, &QNetworkReply::finished, this, &UpdateChecker::onDownloadFinished);
    connect(dlReply_, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
                emit downloadProgress(total > 0 ? int(received * 100 / total) : 0);
            });
}

void UpdateChecker::onDownloadReadyRead()
{
    if (!dlReply_ || !dlFile_) return;
    const QByteArray chunk = dlReply_->readAll();
    if (!chunk.isEmpty()) dlFile_->write(chunk);
}

void UpdateChecker::onDownloadFinished()
{
    QNetworkReply *reply = dlReply_;
    dlReply_ = nullptr;
    if (!reply) return;

    const bool ok = reply->error() == QNetworkReply::NoError;
    const QString errText = reply->errorString();
    reply->deleteLater();

    if (dlFile_) {
        dlFile_->write(reply->readAll());
        dlFile_->close();
        dlFile_->deleteLater();
        dlFile_ = nullptr;
    }

    if (!ok) {
        QFile::remove(dlPath_);
        emit failed(tr("下载安装包失败：") + errText);
        return;
    }
    emit downloadProgress(100);
    emit downloadFinished(dlPath_);
}
