#include "settings.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLinearGradient>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QToolButton>
#include <QVBoxLayout>

#include "state.h"
#include "themes.h"
#include "updater.h"

namespace {

// 勾选框：选中后蓝色底 + 白色对号，避免深色对号看不清
class WhiteCheckBox : public QCheckBox
{
public:
    explicit WhiteCheckBox(QWidget *parent = nullptr) : QCheckBox(parent)
    {
        setFixedSize(20, 20);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool on = isChecked();
        const QRectF box(0.8, 0.8, 18.4, 18.4);

        QColor border = QColor("#C6D0DF");
        if (on) border = QColor("#2F6BFF");
        else if (underMouse()) border = QColor("#9DBBF0");

        p.setPen(QPen(border, 1.3));
        p.setBrush(on ? QColor("#2F6BFF") : QColor("#FFFFFF"));
        p.drawRoundedRect(box, 5, 5);

        if (on) {
            QPainterPath check;
            check.moveTo(5.0, 10.2);
            check.lineTo(8.4, 13.5);
            check.lineTo(15.0, 6.6);
            p.setPen(QPen(Qt::white, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawPath(check);
        }
    }
};

// 外观缩略图：按真实挂绳的样子画——一根轻微下垂的绳子，挂着三张带木夹的图片卡
QPixmap themePreview(const LineTheme &t, const QSize &size)
{
    const qreal dpr = 2.0;
    QPixmap pm(size * dpr);
    pm.setDevicePixelRatio(dpr);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const qreal w = size.width();
    const qreal h = size.height();
    const QRectF r(0, 0, w, h);

    // 背景：像桌面一样的暖灰
    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0, QColor("#F4F3EF"));
    bg.setColorAt(1, QColor("#E8E6E0"));
    p.setPen(QPen(QColor("#DFDDD6"), 1));
    p.setBrush(bg);
    p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);

    // 绳子：与挂绳窗口一致的轻微下垂
    const qreal ropeY = 23;
    const qreal x1 = 10;
    const qreal x2 = w - 10;
    const qreal rw = qMax<qreal>(2.0, t.ropeWidth * 0.8);
    const qreal sag = qMin<qreal>(5.0, (x2 - x1) * 0.05);

    QPainterPath rope;
    rope.moveTo(x1, ropeY);
    rope.quadTo((x1 + x2) / 2.0, ropeY + sag * 2.0, x2, ropeY);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(16, 24, 38, 30), rw + 1.4, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(rope.translated(0, 1.6));
    p.setPen(QPen(t.ropeColor, rw, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(rope);
    p.setPen(QPen(t.ropeHighlight, qMax<qreal>(0.8, rw / 2.6), Qt::SolidLine, Qt::RoundCap));
    p.drawPath(rope.translated(0, -0.6));

    // 左端小钉
    p.setPen(Qt::NoPen);
    p.setBrush(t.ropeColor.darker(125));
    p.drawEllipse(QPointF(x1, ropeY), 2.2, 2.2);
    p.setBrush(QColor(255, 255, 255, 110));
    p.drawEllipse(QPointF(x1 - 0.6, ropeY - 0.7), 0.9, 0.9);

    // 三张悬挂的卡片：照片用不同的柔和色调区分
    struct Spec { qreal cx; qreal baseY; qreal tilt; QColor top; QColor bottom; };
    const Spec specs[3] = {
        { 46,  28.0, -3.0, QColor("#BBD8F3"), QColor("#E9F3FC") },
        { 86,  29.5,  1.6, QColor("#F4D2B6"), QColor("#FCEFE3") },
        { 126, 27.5,  3.0, QColor("#C9E0C4"), QColor("#EDF7E9") },
    };

    for (int i = 0; i < 3; ++i) {
        const Spec &s = specs[i];
        p.save();
        p.translate(s.cx, s.baseY);
        p.rotate(s.tilt);

        const QRectF card(-20, 0, 40, 52);

        // 投影
        for (int k = 4; k >= 1; --k) {
            QColor c(20, 28, 44);
            c.setAlpha(5 + (4 - k) * 2);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(card.adjusted(-k * 0.5, -k * 0.3 + 2.4, k * 0.5, k * 0.7 + 2.4),
                              4 + k * 0.5, 4 + k * 0.5);
        }

        // 卡片主体
        p.setPen(QPen(QColor("#E2E7EF"), 0.9));
        p.setBrush(QColor(253, 253, 255));
        p.drawRoundedRect(card, 4, 4);

        // 照片区
        const QRectF img = card.adjusted(3.2, 9.5, -3.2, -3.2);
        QLinearGradient ph(img.topLeft(), img.bottomRight());
        ph.setColorAt(0, s.top);
        ph.setColorAt(1, s.bottom);
        p.setPen(Qt::NoPen);
        p.setBrush(ph);
        p.drawRoundedRect(img, 2.6, 2.6);

        // 照片里的示意元素（窗口圆点 + 两条文字线）
        p.setBrush(QColor(255, 255, 255, 150));
        p.drawEllipse(QPointF(img.left() + 4.0, img.top() + 4.0), 1.1, 1.1);
        p.setPen(QPen(QColor(255, 255, 255, 135), 1.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(img.left() + 3.2, img.top() + 10.0),
                   QPointF(img.right() - 9.0, img.top() + 10.0));
        p.drawLine(QPointF(img.left() + 3.2, img.top() + 15.0),
                   QPointF(img.right() - 14.0, img.top() + 15.0));

        // 木夹：夹头 + 两片夹脚，夹在绳子与卡片之间
        const QColor clip = t.clipColors.isEmpty()
            ? t.ropeColor : t.clipColors.at(i % t.clipColors.size());

        QLinearGradient head(QPointF(-8, -3), QPointF(-8, 8));
        head.setColorAt(0.0, clip.lighter(126));
        head.setColorAt(0.55, clip.lighter(104));
        head.setColorAt(1.0, clip.darker(108));
        p.setPen(QPen(clip.darker(128), 0.9));
        p.setBrush(head);
        p.drawRoundedRect(QRectF(-8, -3, 16, 11), 4, 4);

        // 金属弹簧圈高光
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 120));
        p.drawRoundedRect(QRectF(-5, -1.6, 6, 1.5), 0.75, 0.75);

        // 两片夹脚
        p.setPen(QPen(clip.darker(125), 0.8));
        QLinearGradient leg(QPointF(-6, 8), QPointF(-6, 16));
        leg.setColorAt(0.0, clip.lighter(112));
        leg.setColorAt(1.0, clip.darker(110));
        p.setBrush(leg);
        p.drawRoundedRect(QRectF(-6.2, 7.5, 5.0, 7.5), 1.8, 1.8);
        p.drawRoundedRect(QRectF(1.2, 7.5, 5.0, 7.5), 1.8, 1.8);

        p.restore();
    }

    return pm;
}

// 分节标题：16px 加粗（QSS 与 QFont 双保险，避免字体样式不生效）
QLabel *makeSectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("section"));
    QFont f = label->font();
    f.setPixelSize(16);
    f.setBold(true);
    label->setFont(f);
    return label;
}

// 主题卡片：自绘预览 + 名称。选中用浅底 + 亮蓝描边，名称加粗加深，始终清晰
class ThemeCard : public QAbstractButton
{
public:
    ThemeCard(const QString &id, const QString &name,
              const QPixmap &preview, QWidget *parent)
        : QAbstractButton(parent), id_(id), name_(name), preview_(preview)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setFixedSize(196, 150);
        setAccessibleName(name);
    }

    QString themeId() const { return id_; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        QColor border = QColor("#E3E8F0");
        if (isChecked()) border = QColor("#6E9AFF");
        else if (underMouse()) border = QColor("#C8D6EC");

        p.setPen(QPen(border, isChecked() ? 2.0 : 1.0));
        p.setBrush(isChecked() ? QColor("#F7FAFF") : QColor("#FFFFFF"));
        p.drawRoundedRect(r, 14, 14);

        const QSize iconSize(172, 90);
        const int ix = (width() - iconSize.width()) / 2;
        const int iy = 10;
        p.drawPixmap(QRect(ix, iy, iconSize.width(), iconSize.height()), preview_);

        QFont f = font();
        f.setPixelSize(13);
        f.setBold(isChecked());
        p.setFont(f);
        p.setPen(isChecked() ? QColor("#173A70") : QColor("#3A4557"));
        p.drawText(QRect(6, iy + iconSize.height() + 7, width() - 12, 22),
                   Qt::AlignHCenter | Qt::AlignVCenter, name_);
    }

private:
    QString id_;
    QString name_;
    QPixmap preview_;
};

} // namespace

SettingsDialog::SettingsDialog(StateStore *store, QWidget *parent)
    : PanelWindow(QStringLiteral("设置"), parent), store_(store)
{
    setFixedSize(720, 640);
    setStyleSheet(styleSheet() + QStringLiteral(
        "QLabel#section{font-size:24px;font-weight:700;color:#1B2330;}"
        "QLabel#rowLabel{font-size:14px;color:#33405A;}"
        "QLabel#percent{font-size:12px;color:#4A5668;}"
        "QLabel#status{font-size:12px;color:#2C8A57;}"
        "QProgressBar{background:#EEF2F8;border:1px solid #E1E7F0;border-radius:6px;}"
        "QProgressBar::chunk{background:#2F6BFF;border-radius:5px;}"));

    QVBoxLayout *root = contentLayout();
    buildAppearance(root);
    root->addSpacing(10);
    buildAbout(root);
    root->addStretch(1);

    updater_ = new UpdateChecker(this);

    connect(updater_, &UpdateChecker::upToDate, this, [this]{
        resetUpdateUi();
        status_->setText(QStringLiteral("当前已是最新版"));
        status_->setVisible(true);
    });
    connect(updater_, &UpdateChecker::failed, this, [this](const QString &msg){
        resetUpdateUi();
        QMessageBox::warning(this, QStringLiteral("检查更新"), msg);
    });
    connect(updater_, &UpdateChecker::updateAvailable, this,
            [this](const QString &v, const QUrl &setupUrl, const QUrl &pageUrl){
        if (setupUrl.isEmpty()) {
            resetUpdateUi();
            QMessageBox::information(
                this, QStringLiteral("发现新版本"),
                QStringLiteral("发现新版本 %1，但该版本没有可自动安装的安装包，"
                               "请前往发布页下载。").arg(v));
            if (!pageUrl.isEmpty()) QDesktopServices::openUrl(pageUrl);
            return;
        }
        beginDownload(setupUrl);
    });
    connect(updater_, &UpdateChecker::downloadProgress, this, [this](int percent){
        if (!downloading_) return;
        progress_->setValue(percent);
        percent_->setText(QStringLiteral("%1%").arg(percent));
    });
    connect(updater_, &UpdateChecker::downloadFinished,
            this, &SettingsDialog::launchInstaller);
}

void SettingsDialog::showCentered()
{
    QScreen *s = QGuiApplication::primaryScreen();
    const QRect wa = s ? s->availableGeometry() : QRect(0, 0, 1280, 800);
    move(wa.center() - QPoint(width() / 2, height() / 2));
    show();
    raise();
    activateWindow();
}

void SettingsDialog::buildAppearance(QVBoxLayout *root)
{
    root->addWidget(makeSectionTitle(QStringLiteral("外观"), this));

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);

    themeGroup_ = new QButtonGroup(this);
    themeGroup_->setExclusive(true);

    const QVector<LineTheme> themes = LineTheme::all();
    for (int i = 0; i < themes.size(); ++i) {
        const LineTheme &t = themes.at(i);
        auto *card = new ThemeCard(t.id, t.name, themePreview(t, QSize(172, 90)), this);
        card->setToolTip(t.description);
        if (t.id == store_->state.theme) card->setChecked(true);
        themeGroup_->addButton(card, i);
        grid->addWidget(card, i / 3, i % 3);
    }
    root->addLayout(grid);

    connect(themeGroup_, &QButtonGroup::idClicked, this, [this](int id){
        auto *card = static_cast<ThemeCard *>(themeGroup_->button(id));
        if (!card) return;
        const QString themeId = card->themeId();
        if (themeId.isEmpty() || store_->state.theme == themeId) return;
        store_->state.theme = themeId;
        store_->save();
        emit themeSelected(themeId);
    });
}

void SettingsDialog::buildAbout(QVBoxLayout *root)
{
    root->addWidget(makeSectionTitle(QStringLiteral("关于"), this));

    // 自动检查更新
    auto *autoRow = new QHBoxLayout;
    autoRow->setContentsMargins(0, 0, 0, 0);
    auto *autoLabel = new QLabel(QStringLiteral("自动检查更新"), this);
    autoLabel->setObjectName(QStringLiteral("rowLabel"));
    autoRow->addWidget(autoLabel);
    autoRow->addStretch(1);

    auto *autoCheck = new WhiteCheckBox(this);
    autoCheck->setChecked(store_->state.autoCheckUpdate);
    autoRow->addWidget(autoCheck);
    connect(autoCheck, &QCheckBox::toggled, this, [this](bool on){
        store_->state.autoCheckUpdate = on;
        store_->save();
    });
    root->addLayout(autoRow);
    root->addSpacing(6);

    // 当前版本 + 检查更新
    auto *verRow = new QHBoxLayout;
    verRow->setContentsMargins(0, 0, 0, 0);
    auto *verLabel = new QLabel(
        QStringLiteral("当前版本为 %1").arg(QCoreApplication::applicationVersion()), this);
    verLabel->setObjectName(QStringLiteral("rowLabel"));
    verRow->addWidget(verLabel);
    verRow->addStretch(1);

    checkBtn_ = new QPushButton(QStringLiteral("检查更新"), this);
    checkBtn_->setCursor(Qt::PointingHandCursor);
    connect(checkBtn_, &QPushButton::clicked, this, &SettingsDialog::startManualCheck);
    verRow->addWidget(checkBtn_);
    root->addLayout(verRow);

    // 检查结果提示（例如“当前已是最新版”），显示在版本行下方
    status_ = new QLabel(this);
    status_->setObjectName(QStringLiteral("status"));
    status_->setVisible(false);
    root->addWidget(status_);

    // 下载进度：默认隐藏，检查到新版本后显示在版本行下方
    auto *progressRow = new QHBoxLayout;
    progressRow->setContentsMargins(0, 0, 0, 0);
    progressRow->setSpacing(10);
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    progress_->setValue(0);
    progress_->setTextVisible(false);
    progress_->setFixedHeight(12);
    progress_->setVisible(false);
    progressRow->addWidget(progress_, 1);

    percent_ = new QLabel(QStringLiteral("0%"), this);
    percent_->setObjectName(QStringLiteral("percent"));
    percent_->setFixedWidth(40);
    percent_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    percent_->setVisible(false);
    progressRow->addWidget(percent_);

    root->addLayout(progressRow);
}

void SettingsDialog::startManualCheck()
{
    if (downloading_) return;

    status_->setVisible(false);
    checkBtn_->setEnabled(false);
    checkBtn_->setText(QStringLiteral("检查中…"));
    updater_->checkLatest();
}

void SettingsDialog::beginDownload(const QUrl &url)
{
    downloading_ = true;
    status_->setVisible(false);
    checkBtn_->setEnabled(false);
    checkBtn_->setText(QStringLiteral("下载中…"));
    progress_->setValue(0);
    percent_->setText(QStringLiteral("0%"));
    progress_->setVisible(true);
    percent_->setVisible(true);
    updater_->download(url);
}

void SettingsDialog::launchInstaller(const QString &filePath)
{
    percent_->setText(QStringLiteral("100%"));
    progress_->setValue(100);
    checkBtn_->setText(QStringLiteral("正在安装…"));

    if (!QProcess::startDetached(filePath)) {
        downloading_ = false;
        resetUpdateUi();
        QMessageBox::warning(this, QStringLiteral("检查更新"),
                             QStringLiteral("无法启动安装程序：%1").arg(filePath));
        return;
    }
    QCoreApplication::quit();
}

void SettingsDialog::resetUpdateUi()
{
    downloading_ = false;
    status_->setVisible(false);
    checkBtn_->setEnabled(true);
    checkBtn_->setText(QStringLiteral("检查更新"));
    progress_->setVisible(false);
    percent_->setVisible(false);
}
