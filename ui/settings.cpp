#include "settings.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCursor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTimer>
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
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        QFont f = font();
        f.setPixelSize(14);
        setFont(f);
    }

    // 勾选框 + 右侧文字：有文字时宽度按文字实际宽度扩展
    QSize sizeHint() const override
    {
        const int tw = text().isEmpty() ? 0 : kTextGap + fontMetrics().horizontalAdvance(text());
        return QSize(20 + tw, qMax(20, fontMetrics().height() + 2));
    }
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave)
            update();
        return QCheckBox::event(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool on = isChecked();
        const qreal top = (height() - 18.4) / 2.0;
        const QRectF box(0.8, top + 0.8, 18.4, 18.4);

        QColor border = QColor("#C6D0DF");
        if (on) border = QColor("#2F6BFF");
        else if (underMouse()) border = QColor("#9DBBF0");

        p.setPen(QPen(border, 1.3));
        p.setBrush(on ? QColor("#2F6BFF") : QColor("#FFFFFF"));
        p.drawRoundedRect(box, 5, 5);

        if (on) {
            // 对号按勾选框中心垂直居中
            p.save();
            p.translate(0, top);
            QPainterPath check;
            check.moveTo(5.0, 10.2);
            check.lineTo(8.4, 13.5);
            check.lineTo(15.0, 6.6);
            p.setPen(QPen(Qt::white, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawPath(check);
            p.restore();
        }

        // 文字由本类自行绘制（不调用基类 paintEvent）
        if (!text().isEmpty()) {
            p.setPen(QColor("#33405A"));
            p.drawText(QRectF(kTextGap, 0, width() - kTextGap, height()),
                       Qt::AlignLeft | Qt::AlignVCenter, text());
        }
    }

private:
    static constexpr int kTextGap = 26;
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

    // 缩略图按 172×90 的设计尺寸绘制，再整体缩放到目标大小，
    // 这样小卡片里绳子、木夹和三张图片的相对位置不会错乱。
    const qreal dw = 172.0;
    const qreal dh = 90.0;
    p.scale(size.width() / dw, size.height() / dh);

    const qreal w = dw;
    const qreal h = dh;
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
        setFixedSize(150, 116);   // 一行四个外观
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
        p.drawRoundedRect(r, 12, 12);

        const QSize iconSize(130, 68);
        const int ix = (width() - iconSize.width()) / 2;
        const int iy = 8;
        p.drawPixmap(QRect(ix, iy, iconSize.width(), iconSize.height()), preview_);

        QFont f = font();
        f.setPixelSize(12);
        f.setBold(isChecked());
        p.setFont(f);
        p.setPen(isChecked() ? QColor("#173A70") : QColor("#3A4557"));
        p.drawText(QRect(6, iy + iconSize.height() + 4, width() - 12, 20),
                   Qt::AlignHCenter | Qt::AlignVCenter, name_);
    }

private:
    QString id_;
    QString name_;
    QPixmap preview_;
};

// 开机自启：读写与安装包完全相同的 HKCU Run 项，
// 这样安装向导里勾选的自启能在这里反映并可关闭，绿色版也能直接开启。
const QLatin1String kRunKey(
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const QLatin1String kRunValue("PicRope");
// 内容区顶部留白：快捷跳转与滚动高亮都按这个值对齐标题位置
constexpr int kContentTop = 15;

bool autoStartEnabled()
{
    QSettings reg(kRunKey, QSettings::NativeFormat);
    return !reg.value(kRunValue).toString().trimmed().isEmpty();
}

void setAutoStartEnabled(bool on)
{
    QSettings reg(kRunKey, QSettings::NativeFormat);
    if (on) {
        // Windows 的 Run 项要求给含空格的路径加引号
        const QString exe = QDir::toNativeSeparators(
            QCoreApplication::applicationFilePath());
        reg.setValue(kRunValue, QStringLiteral("\"%1\"").arg(exe));
    } else {
        reg.remove(kRunValue);
    }
    reg.sync();
}

} // namespace

SettingsDialog::SettingsDialog(StateStore *store, QWidget *parent)
    : PanelWindow(tr("设置"), parent), store_(store)
{
    setFixedSize(720, 740);   // 与加入本次新功能前的设置窗口尺寸一致
    configureHeader(40, 16, 28);            // 标题行高 40，与顶部底色高度一致
    setHeaderTint(QColor("#ECECEE"), 40);   // 标题栏底色：到标题行下方、内容区开始处为止
    // 标题栏与内容区之间不留间隙，留白全部由内容区上边距提供
    if (auto *v = qobject_cast<QVBoxLayout *>(layout()))
        v->setSpacing(0);
    setStyleSheet(styleSheet() + QStringLiteral(
        "QLabel#section{font-size:24px;font-weight:700;color:#1B2330;}"
        "QLabel#rowLabel{font-size:14px;color:#33405A;}"
        "QLabel#percent{font-size:12px;color:#4A5668;}"
        "QLabel#status{font-size:12px;color:#2C8A57;}"
        "QComboBox{background:#FFFFFF;border:1px solid #DCE2EC;border-radius:8px;"
        "padding:4px 10px;color:#33405A;min-width:110px;}"
        "QComboBox:hover{border-color:#C9D3E2;}"
        "QComboBox QAbstractItemView{background:#FFFFFF;border:1px solid #DCE2EC;"
        "selection-background-color:#E7EFFC;selection-color:#16345F;outline:none;}"
        "QKeySequenceEdit QLineEdit{background:#FFFFFF;border:1px solid #DCE2EC;"
        "border-radius:8px;padding:4px 10px;color:#33405A;}"
        "QProgressBar{background:#EEF2F8;border:1px solid #E1E7F0;border-radius:6px;}"
        "QProgressBar::chunk{background:#2F6BFF;border-radius:5px;}"
        "QScrollArea{background:#FDFEFF;border:none;}"
        "QScrollArea > QWidget > QWidget{background:#FDFEFF;}"
        "QWidget#settingsBody{background:#FDFEFF;}"
        "QScrollBar:vertical{background:transparent;width:8px;margin:2px 0;}"
        "QPushButton#navBtn{background:transparent;border:none;border-radius:8px;"
        "padding:8px 12px;text-align:left;color:#3A475C;font-size:14px;}"
        "QPushButton#navBtn:hover{background:#EDF2FA;}"
        "QPushButton#navBtn:checked{background:#E6EEFC;color:#1D4FD8;font-weight:600;}"
        "QScrollBar::handle:vertical{background:#D2DAE8;border-radius:4px;min-height:30px;}"
        "QScrollBar::handle:vertical:hover{background:#BFC9DA;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"));

    // 左右两栏：左侧分区导航，右侧滚动内容
    auto *split = new QWidget(this);
    auto *splitLayout = new QHBoxLayout(split);
    splitLayout->setContentsMargins(0, 0, 0, 0);
    splitLayout->setSpacing(14);

    // 内容放进滚动区：窗口高度不够时上下滑动，不再为了塞下内容拉高窗口
    scroll_ = new QScrollArea(split);
    scroll_->setObjectName(QStringLiteral("settingsScroll"));
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_->viewport()->setAutoFillBackground(false);
    // 设置窗口本身是透明窗口：滚动区、内容容器都必须画白底，否则会透出桌面
    scroll_->setAutoFillBackground(true);
    QPalette scrollPal = scroll_->palette();
    scrollPal.setColor(QPalette::Window, QColor("#FDFEFF"));
    scroll_->setPalette(scrollPal);

    auto *body = new QWidget;
    body->setObjectName(QStringLiteral("settingsBody"));
    // 只对容器自身设背景，避免级联把下拉弹层也刷成透明
    body->setAutoFillBackground(true);
    QPalette bodyPal = body->palette();
    bodyPal.setColor(QPalette::Window, QColor("#FDFEFF"));
    body->setPalette(bodyPal);
    auto *root = new QVBoxLayout(body);
    root->setContentsMargins(0, kContentTop, 18, 0);   // 内容区上边距（与快捷跳转的定位基准一致）
    root->setSpacing(12);

    buildFeatures(root);
    root->addSpacing(10);
    buildShortcuts(root);
    root->addSpacing(10);
    buildAppearance(root);
    root->addSpacing(10);
    buildData(root);
    root->addSpacing(10);
    buildOther(root);
    root->addSpacing(10);
    buildAbout(root);
    root->addStretch(1);

    scroll_->setWidget(body);
    splitLayout->addWidget(buildNav(), 0);
    splitLayout->addWidget(scroll_, 1);
    contentLayout()->addWidget(split, 1);

    updater_ = new UpdateChecker(this);

    connect(updater_, &UpdateChecker::upToDate, this, [this]{
        resetUpdateUi();
        status_->setText(tr("当前已是最新版"));
        status_->setVisible(true);
        // 仅作短暂提示，3 秒后自动消失；若期间又开始检查/下载则不清空新状态
        QTimer::singleShot(3000, status_, [this]{
            if (status_->text() == tr("当前已是最新版"))
                status_->setVisible(false);
        });
    });
    connect(updater_, &UpdateChecker::failed, this, [this](const QString &msg){
        resetUpdateUi();
        QMessageBox::warning(this, tr("检查更新"), msg);
    });
    connect(updater_, &UpdateChecker::updateAvailable, this,
            [this](const QString &v, const QUrl &setupUrl, const QUrl &pageUrl){
        if (setupUrl.isEmpty()) {
            resetUpdateUi();
            QMessageBox::information(
                this, tr("发现新版本"),
                tr("发现新版本 %1，但该版本没有可自动安装的安装包，"
                   "请前往发布页下载。").arg(v));
            if (!pageUrl.isEmpty()) QDesktopServices::openUrl(pageUrl);
            return;
        }
        beginDownload(setupUrl);
    });
    connect(updater_, &UpdateChecker::downloadProgress, this, [this](int percent){
        if (!downloading_) return;
        progress_->setValue(percent);
        percent_->setText(tr("%1%").arg(percent));
    });
    connect(updater_, &UpdateChecker::downloadFinished,
            this, &SettingsDialog::launchInstaller);
}

void SettingsDialog::showCentered()
{
    syncFromState();   // 设置界面复用，打开前同步托盘那边可能已改过的开关
    // 多屏下以鼠标所在屏幕居中，而不是固定主屏
    QScreen *s = QGuiApplication::screenAt(QCursor::pos());
    if (!s) s = QGuiApplication::primaryScreen();
    const QRect wa = s ? s->availableGeometry() : QRect(0, 0, 1280, 800);
    move(wa.center() - QPoint(width() / 2, height() / 2));   // 固定尺寸，居中即可
    show();
    raise();
    activateWindow();
}

void SettingsDialog::syncFromState()
{
    if (startCheck_) {
        // 自启状态以注册表为准，可能被安装包或系统工具改动过
        const QSignalBlocker block(startCheck_);
        startCheck_->setChecked(autoStartEnabled());
    }
    if (!edgeCheck_) return;
    const QSignalBlocker block(edgeCheck_);   // 仅同步显示，不触发保存与信号
    edgeCheck_->setChecked(store_->state.edgePopup);

    // 托盘侧可能改过的开关与下拉，这里一并同步（不触发回写）
    auto syncCheck = [](QCheckBox *box, bool on){
        if (!box) return;
        const QSignalBlocker b(box);
        box->setChecked(on);
    };
    syncCheck(clipCheck_, store_->state.collectClipboard);
    syncCheck(textCheck_, store_->state.collectTextClipboard);
    syncCheck(hangCheck_, store_->state.autoHang);

    auto syncCombo = [](QComboBox *combo, int value){
        if (!combo) return;
        const QSignalBlocker b(combo);
        int idx = combo->findData(value);
        if (idx < 0 && combo->count() > 0) {
            int best = 0;
            int bestDiff = -1;
            for (int i = 0; i < combo->count(); ++i) {
                const int d = qAbs(combo->itemData(i).toInt() - value);
                if (bestDiff < 0 || d < bestDiff) { bestDiff = d; best = i; }
            }
            idx = best;
        }
        combo->setCurrentIndex(idx >= 0 ? idx : 0);
    };
    syncCombo(sizeCombo_, store_->state.cardSize);
    syncCombo(opacityCombo_, store_->state.shelfOpacity);
    syncCombo(delayCombo_, store_->state.hideDelayMs);

    if (hotkeyEdit_) {
        const QSignalBlocker b(hotkeyEdit_);
        hotkeyEdit_->setKeySequence(QKeySequence(store_->state.toggleHotkey));
    }

    if (cleanCheck_) {
        const QSignalBlocker b(cleanCheck_);
        cleanCheck_->setChecked(store_->state.autoClean);
        if (limitBox_) limitBox_->setVisible(store_->state.autoClean);
    }
    if (imageLimitSpin_) {
        const QSignalBlocker b(imageLimitSpin_);
        imageLimitSpin_->setValue(store_->state.imageLimit);
    }
    if (textLimitSpin_) {
        const QSignalBlocker b(textLimitSpin_);
        textLimitSpin_->setValue(store_->state.textLimit);
    }
}

QWidget *SettingsDialog::buildNav()
{
    auto *nav = new QWidget(this);
    nav->setObjectName(QStringLiteral("settingsNav"));
    nav->setFixedWidth(128);
    auto *lay = new QVBoxLayout(nav);
    lay->setContentsMargins(0, 15, 0, 0);   // 左侧导航整体下移
    lay->setSpacing(4);

    navGroup_ = new QButtonGroup(this);
    navGroup_->setExclusive(true);

    const QVector<QPair<QString, QString>> items = {
        {QStringLiteral("features"),   tr("功能")},
        {QStringLiteral("shortcuts"),  tr("快捷键")},
        {QStringLiteral("appearance"), tr("外观")},
        {QStringLiteral("data"),       tr("数据")},
        {QStringLiteral("other"),      tr("其他")},
        {QStringLiteral("about"),      tr("关于")},
    };
    int id = 0;
    for (const auto &it : items) {
        auto *btn = new QPushButton(it.second, nav);
        btn->setObjectName(QStringLiteral("navBtn"));
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        navGroup_->addButton(btn, id++);
        lay->addWidget(btn);
        const QString key = it.first;
        connect(btn, &QPushButton::clicked, this, [this, key]{
            // 点击后短暂锁定高亮：否则滚动到目标位置时，滚动信号会按当前
            // 位置把高亮改回上一分区（数据 / 关于位于底部时尤其明显）
            navPinning_ = true;
            QTimer::singleShot(400, this, [this]{ navPinning_ = false; });
            scrollToSection(key);
        });
    }
    lay->addStretch(1);
    if (auto *first = navGroup_->button(0)) first->setChecked(true);

    // 滚动时根据当前位置高亮对应分区
    if (scroll_) {
        connect(scroll_->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v){
            if (navPinning_) return;   // 点击跳转的锁定期内不按位置改高亮
            const QStringList keys = {QStringLiteral("features"), QStringLiteral("shortcuts"),
                                      QStringLiteral("appearance"), QStringLiteral("data"),
                                      QStringLiteral("other"), QStringLiteral("about")};
            int best = 0;
            for (int i = 0; i < keys.size(); ++i) {
                QWidget *w = anchors_.value(keys.at(i));
                if (w && w->y() - kContentTop <= v) best = i;
            }
            // 滚到底部时，标题可能仍在视口下方，按可视范围补选最后一个
            auto *bar = scroll_->verticalScrollBar();
            if (bar->value() >= bar->maximum() - 2) {
                const int bottom = v + scroll_->viewport()->height();
                for (int i = 0; i < keys.size(); ++i) {
                    QWidget *w = anchors_.value(keys.at(i));
                    if (w && w->y() <= bottom) best = i;
                }
            }
            if (navGroup_ && navGroup_->button(best))
                navGroup_->button(best)->setChecked(true);
        });
    }
    return nav;
}

void SettingsDialog::scrollToSection(const QString &key)
{
    QWidget *w = anchors_.value(key);
    if (!scroll_ || !w) return;
    scroll_->verticalScrollBar()->setValue(qMax(0, w->y() - kContentTop));
}

QVBoxLayout *SettingsDialog::addSectionCard(QVBoxLayout *root)
{
    // 每个设置分区一个浅色圆角框，内部行距统一，避免各分区疏密不一
    auto *card = new QWidget(this);
    card->setObjectName(QStringLiteral("sectionCard"));
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(QStringLiteral(
        "QWidget#sectionCard{background:#F4F7FB;border:1px solid #D8E0EB;"
        "border-radius:10px;}"));
    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 12, 16, 12);
    lay->setSpacing(10);
    root->addWidget(card);
    return lay;
}

void SettingsDialog::buildFeatures(QVBoxLayout *root)
{
    auto *title = makeSectionTitle(tr("功能"), this);
    anchors_.insert(QStringLiteral("features"), title);
    root->addWidget(title);
    root = addSectionCard(root);

    // 鼠标移到屏幕顶部自动弹出挂绳：与托盘菜单共用同一份状态，改动会持久化
    auto *edgeRow = new QHBoxLayout;
    edgeRow->setContentsMargins(0, 0, 0, 0);
    auto *edgeLabel = new QLabel(
        tr("鼠标移到屏幕顶部自动弹出挂绳"), this);
    edgeLabel->setObjectName(QStringLiteral("rowLabel"));
    edgeRow->addWidget(edgeLabel);
    edgeRow->addStretch(1);

    auto *edgeCheck = new WhiteCheckBox(this);
    edgeCheck_ = edgeCheck;
    edgeCheck->setChecked(store_->state.edgePopup);
    edgeRow->addWidget(edgeCheck);
    connect(edgeCheck, &QCheckBox::toggled, this, [this](bool on){
        store_->state.edgePopup = on;
        store_->save();
        emit edgePopupChanged(on);
    });
    root->addLayout(edgeRow);

    // 剪贴板收集与新图片自动上绳：一行一项，标签在左、勾选框在右，与其它开关样式一致
    auto addSwitchRow = [this, root](const QString &text, bool on) {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        auto *lab = new QLabel(text, this);
        lab->setObjectName(QStringLiteral("rowLabel"));
        row->addWidget(lab);
        row->addStretch(1);
        auto *box = new WhiteCheckBox(this);
        box->setChecked(on);
        row->addWidget(box);
        root->addLayout(row);
        return box;
    };

    clipCheck_ = addSwitchRow(tr("剪贴板收集 · 图片"), store_->state.collectClipboard);
    connect(clipCheck_, &QCheckBox::toggled, this, [this](bool on){
        emit collectClipboardChanged(on);
    });

    textCheck_ = addSwitchRow(tr("剪贴板收集 · 文字"), store_->state.collectTextClipboard);
    connect(textCheck_, &QCheckBox::toggled, this, [this](bool on){
        emit collectTextClipboardChanged(on);
    });

    hangCheck_ = addSwitchRow(tr("新图片自动上绳"), store_->state.autoHang);
    connect(hangCheck_, &QCheckBox::toggled, this, [this](bool on){
        emit autoHangChanged(on);
    });

    // 开机自启：状态以注册表 Run 项为准，因此安装版在安装向导里勾选的自启
    // 在这里会显示为已勾选，取消勾选即可关闭；绿色版也能直接用这里开启。
    auto *startRow = new QHBoxLayout;
    startRow->setContentsMargins(0, 0, 0, 0);
    auto *startLabel = new QLabel(tr("开机时自动启动图绳"), this);
    startLabel->setObjectName(QStringLiteral("rowLabel"));
    startRow->addWidget(startLabel);
    startRow->addStretch(1);

    auto *startCheck = new WhiteCheckBox(this);
    startCheck_ = startCheck;
    startCheck->setChecked(autoStartEnabled());
    startRow->addWidget(startCheck);
    connect(startCheck, &QCheckBox::toggled, this, [](bool on){
        setAutoStartEnabled(on);
    });
    root->addLayout(startRow);

    // 界面语言：默认简体中文；切换后提示重启生效
    auto *langRow = new QHBoxLayout;
    langRow->setContentsMargins(0, 0, 0, 0);
    auto *langLabel = new QLabel(tr("界面语言"), this);
    langLabel->setObjectName(QStringLiteral("rowLabel"));
    langRow->addWidget(langLabel);
    langRow->addStretch(1);

    auto *combo = new QComboBox(this);
    combo->addItem(tr("简体中文"), QStringLiteral("zh_CN"));
    combo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    // 按当前持久化语言选中
    {
        const QString cur = store_->state.language;
        const int idx = combo->findData(cur);
        combo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    combo->setCursor(Qt::PointingHandCursor);
    langRow->addWidget(combo);
    connect(combo, &QComboBox::currentIndexChanged, this,
            [this, combo, prevIndex = combo->currentIndex()](int idx) mutable {
        const QString lang = combo->itemData(idx).toString();
        if (store_->state.language == lang) { prevIndex = idx; return; }

        QMessageBox box(QMessageBox::Question, tr("界面语言"),
                        tr("界面语言将在重启后生效，是否立即重启图绳？"),
                        QMessageBox::Yes | QMessageBox::No);
        box.setDefaultButton(QMessageBox::Yes);
        if (box.exec() != QMessageBox::Yes) {
            // 选择 No 视为取消：不保存，并把下拉框恢复为切换前的选项。
            // 这里必须延迟到本次鼠标事件处理完再恢复，否则弹出列表内部的当前项
            // 会被事件收尾流程覆盖，表现为再次展开时高亮仍是取消掉的那一项。
            QTimer::singleShot(0, combo, [combo, prevIndex]{
                const QSignalBlocker blocker(combo);
                combo->setCurrentIndex(prevIndex);
                if (QAbstractItemView *view = combo->view()) {
                    const QSignalBlocker viewBlocker(view->selectionModel());
                    view->setCurrentIndex(combo->model()->index(prevIndex,
                                                               combo->modelColumn()));
                }
            });
            return;
        }

        prevIndex = idx;
        store_->state.language = lang;
        store_->save();

        QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                QStringList());
        QCoreApplication::quit();
    });
    root->addLayout(langRow);

    // 自动清理：勾选后在下方展开图片与文字的数量上限，默认各 200
    auto *cleanRow = new QHBoxLayout;
    cleanRow->setContentsMargins(0, 0, 0, 0);
    auto *cleanLabel = new QLabel(tr("自动清理"), this);
    cleanLabel->setObjectName(QStringLiteral("rowLabel"));
    cleanRow->addWidget(cleanLabel);
    cleanRow->addStretch(1);

    auto *cleanCheck = new WhiteCheckBox(this);
    cleanCheck_ = cleanCheck;
    cleanCheck->setChecked(store_->state.autoClean);
    cleanRow->addWidget(cleanCheck);
    root->addLayout(cleanRow);

    auto *limitBox = new QWidget(this);
    limitBox_ = limitBox;
    auto *limitLayout = new QVBoxLayout(limitBox);
    limitLayout->setContentsMargins(16, 0, 0, 0);
    limitLayout->setSpacing(10);

    auto makeLimitRow = [this, limitLayout](const QString &text, int value) -> QSpinBox * {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        auto *lab = new QLabel(text, this);
        lab->setObjectName(QStringLiteral("rowLabel"));
        row->addWidget(lab);
        row->addStretch(1);
        auto *spin = new QSpinBox(this);
        spin->setRange(1, 99999);
        spin->setSingleStep(10);
        spin->setValue(value);
        spin->setFixedWidth(110);
        spin->setAlignment(Qt::AlignRight);
        spin->installEventFilter(this);   // 屏蔽滚轮改值，避免滚动设置页时误改
        row->addWidget(spin);
        limitLayout->addLayout(row);
        return spin;
    };
    imageLimitSpin_ = makeLimitRow(tr("图片篮子上限"), store_->state.imageLimit);
    textLimitSpin_ = makeLimitRow(tr("文字笔记上限"), store_->state.textLimit);
    limitBox->setVisible(store_->state.autoClean);
    root->addWidget(limitBox);

    connect(cleanCheck, &QCheckBox::toggled, this, [this](bool on){
        store_->state.autoClean = on;
        store_->save();
        if (limitBox_) limitBox_->setVisible(on);
    });
    connect(imageLimitSpin_, &QSpinBox::editingFinished, this, [this]{ applyLimitChange(true); });
    connect(textLimitSpin_, &QSpinBox::editingFinished, this, [this]{ applyLimitChange(false); });
}

void SettingsDialog::applyLimitChange(bool images)
{
    if (limitPrompting_) return;   // 弹窗获得焦点会再次触发 editingFinished，避免重复弹窗
    QSpinBox *spin = images ? imageLimitSpin_ : textLimitSpin_;
    if (!spin) return;
    const int value = spin->value();
    int &limit = images ? store_->state.imageLimit : store_->state.textLimit;
    if (limit == value) return;

    const int current = images ? store_->state.collection.size()
                               : store_->state.texts.size();
    if (current > value) {
        // 当前数量已超过新上限：先确认，选「否」则取消本次修改
        const QString text = images
            ? tr("当前已存储 %1 张图片，超过新的上限 %2。是否确定修改？")
                  .arg(current).arg(value)
            : tr("当前已存储 %1 条文字，超过新的上限 %2。是否确定修改？")
                  .arg(current).arg(value);
        QMessageBox box(QMessageBox::Question, tr("自动清理"), text,
                        QMessageBox::Yes | QMessageBox::No);
        box.setDefaultButton(QMessageBox::No);
        limitPrompting_ = true;
        const int ret = box.exec();
        limitPrompting_ = false;
        if (ret != QMessageBox::Yes) {
            const QSignalBlocker blocker(spin);
            spin->setValue(limit);   // 取消修改，恢复原上限
            return;
        }
    }

    limit = value;
    store_->save();
}

bool SettingsDialog::eventFilter(QObject *watched, QEvent *event)
{
    // 上限数值框：忽略滚轮，避免在设置页滚动时误改数值
    if ((watched == imageLimitSpin_ || watched == textLimitSpin_)
        && event->type() == QEvent::Wheel) {
        return true;
    }
    if (watched == hotkeyEdit_ && event->type() == QEvent::FocusIn) {
        // 点击聚焦后清空，居中提示等待用户按下新键
        const QSignalBlocker blocker(hotkeyEdit_);
        hotkeyEdit_->clear();
        applyHotkeyPlaceholder();
    } else if (watched == hotkeyEdit_ && event->type() == QEvent::FocusOut) {
        QTimer::singleShot(0, this, [this]{ applyHotkeyPlaceholder(); });
    } else if (watched == hotkeyLine_ && event->type() == QEvent::Resize) {
        if (hotkeyHint_) hotkeyHint_->setGeometry(hotkeyLine_->rect());
    } else if (watched == hotkeyEdit_
               && (event->type() == QEvent::KeyPress
                   || event->type() == QEvent::KeyRelease)) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        Qt::KeyboardModifier modifier = Qt::NoModifier;
        switch (keyEvent->key()) {
        case Qt::Key_Control: modifier = Qt::ControlModifier; break;
        case Qt::Key_Alt:     modifier = Qt::AltModifier; break;
        case Qt::Key_Shift:   modifier = Qt::ShiftModifier; break;
        case Qt::Key_Meta:    modifier = Qt::MetaModifier; break;
        default: break;
        }

        if (modifier != Qt::NoModifier) {
            // QKeySequenceEdit 默认不显示单独的修饰键，先让原逻辑处理，再补充实时预览。
            Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
            if (event->type() == QEvent::KeyPress)
                modifiers |= modifier;
            else
                modifiers &= ~modifier;
            QTimer::singleShot(0, this, [this, modifiers]{
                if (!hotkeyLine_ || !hotkeyEdit_->hasFocus()
                    || !hotkeyEdit_->keySequence().isEmpty()) return;
                QStringList keys;
                if (modifiers & Qt::ControlModifier) keys << QStringLiteral("Ctrl");
                if (modifiers & Qt::AltModifier)     keys << QStringLiteral("Alt");
                if (modifiers & Qt::ShiftModifier)   keys << QStringLiteral("Shift");
                if (modifiers & Qt::MetaModifier)    keys << QStringLiteral("Win");
                hotkeyLine_->setText(keys.join(QStringLiteral("+")));
                applyHotkeyPlaceholder();
            });
        } else if (event->type() == QEvent::KeyPress
                   && keyEvent->key() != Qt::Key_unknown) {
            // 等 QKeySequenceEdit 收下主键后立即退出编辑，后续按键不再被录入。
            QTimer::singleShot(0, hotkeyEdit_, [this]{ hotkeyEdit_->clearFocus(); });
        }
    }
    return PanelWindow::eventFilter(watched, event);
}

void SettingsDialog::applyHotkeyPlaceholder()
{
    if (!hotkeyEdit_) return;
    if (!hotkeyLine_) {
        hotkeyLine_ = hotkeyEdit_->findChild<QLineEdit *>();
        if (hotkeyLine_)
            hotkeyLine_->installEventFilter(this);
    }
    if (!hotkeyLine_) return;

    if (!hotkeyHint_) {
        // Qt 在聚焦且居中对齐时不绘制 QLineEdit 占位提示，因此用透明子标签绘制。
        hotkeyHint_ = new QLabel(tr("请按下快捷键…"), hotkeyLine_);
        hotkeyHint_->setAlignment(Qt::AlignCenter);
        hotkeyHint_->setFocusPolicy(Qt::NoFocus);
        hotkeyHint_->setAttribute(Qt::WA_TransparentForMouseEvents);
        hotkeyHint_->setStyleSheet(QStringLiteral(
            "QLabel{background:transparent;color:#98A2B3;}"));
    }
    hotkeyLine_->setPlaceholderText(QString());
    hotkeyLine_->setAlignment(Qt::AlignCenter);
    hotkeyHint_->setFont(hotkeyLine_->font());
    hotkeyHint_->setGeometry(hotkeyLine_->rect());
    const bool showHint = hotkeyLine_->text().isEmpty() && hotkeyEdit_->hasFocus();
    hotkeyHint_->setVisible(showHint);
    if (showHint) hotkeyHint_->raise();
}

void SettingsDialog::buildShortcuts(QVBoxLayout *root)
{
    auto *title = makeSectionTitle(tr("快捷键"), this);
    anchors_.insert(QStringLiteral("shortcuts"), title);
    root->addWidget(title);
    root = addSectionCard(root);

    auto addRow = [this, root](const QString &name, QWidget *value) {
        auto *r = new QHBoxLayout;
        r->setContentsMargins(0, 0, 0, 0);
        auto *n = new QLabel(name, this);
        n->setObjectName(QStringLiteral("rowLabel"));
        r->addWidget(n);
        r->addStretch(1);
        r->addWidget(value);
        root->addLayout(r);
    };

    // 五条快捷键行统一高度：以只读按键框的自然高度为准，再把可修改的输入框压到同高
    const QString boxStyle = QStringLiteral(
        "QLabel{background:#F4F7FC;border:1px solid #E1E6EE;border-radius:8px;"
        "padding:4px 12px;color:#6B7486;}");
    int rowH = 0;
    {
        QLabel probe(QStringLiteral("Ag"), this);
        probe.setStyleSheet(boxStyle);
        rowH = probe.sizeHint().height();
    }

    // 不可修改的快捷键：只读按键框 + 锁标，鼠标悬停给出说明提示
    auto keyBox = [this, &boxStyle](const QString &keys, const QString &tip) -> QWidget * {
        auto *wrap = new QWidget(this);
        auto *h = new QHBoxLayout(wrap);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(6);
        auto *lab = new QLabel(keys, wrap);
        lab->setAlignment(Qt::AlignCenter);
        lab->setMinimumWidth(150);
        lab->setStyleSheet(boxStyle);
        auto *lock = new QLabel(QStringLiteral("\U0001F512"), wrap);
        lock->setStyleSheet(QStringLiteral("color:#9AA4B2;"));
        h->addWidget(lab);
        h->addWidget(lock);
        wrap->setToolTip(tip);
        lab->setToolTip(tip);
        lock->setToolTip(tip);
        return wrap;
    };

    // 可修改：显示/收起挂绳（默认 Ctrl+Alt+T）
    auto *edit = new QKeySequenceEdit(this);
    hotkeyEdit_ = edit;
    edit->setMaximumSequenceLength(1);
    edit->setClearButtonEnabled(false);
    edit->setFixedWidth(170);
    edit->setCursor(Qt::PointingHandCursor);
    edit->setKeySequence(QKeySequence(store_->state.toggleHotkey));
    if (rowH > 0) edit->setFixedHeight(rowH);   // 与下方四项等高，避免首行偏高
    applyHotkeyPlaceholder();
    edit->installEventFilter(this);
    addRow(tr("显示 / 收起挂绳"), edit);

    // 不可修改：系统默认与查看器内置
    addRow(tr("区域截图"), keyBox(QStringLiteral("Win + Shift + S"),
        tr("系统默认截图快捷键，无法在此修改，可在 Windows 设置中更改。")));
    addRow(tr("图片查看器 · 切换图片"), keyBox(QStringLiteral("← / →"),
        tr("图片查看器内置快捷键，无法在此修改。")));
    addRow(tr("图片查看器 · 缩放复位"), keyBox(QStringLiteral("0"),
        tr("图片查看器内置快捷键，无法在此修改。")));
    addRow(tr("图片查看器 · 关闭"), keyBox(QStringLiteral("Esc"),
        tr("图片查看器内置快捷键，无法在此修改。")));

    connect(edit, &QKeySequenceEdit::editingFinished, this, [this, edit]{
        edit->clearFocus();   // 完整组合键或取消录入后均退出编辑，避免继续捕获按键
        const QKeySequence seq = edit->keySequence();
        auto restore = [this, edit]{
            const QSignalBlocker b(edit);
            edit->setKeySequence(QKeySequence(store_->state.toggleHotkey));
            applyHotkeyPlaceholder();
        };
        if (seq.isEmpty()) { restore(); return; }   // 清空视为取消，保持原热键可用
        if (!(seq[0].keyboardModifiers() & (Qt::ControlModifier | Qt::AltModifier
                                            | Qt::ShiftModifier | Qt::MetaModifier))) {
            QMessageBox::warning(this, tr("快捷键"),
                                 tr("全局热键需要包含 Ctrl、Alt、Shift 或 Win 修饰键。"));
            restore();
            return;
        }
        const QString text = seq.toString(QKeySequence::PortableText);
        if (text == store_->state.toggleHotkey) { applyHotkeyPlaceholder(); return; }
        store_->state.toggleHotkey = text;
        store_->save();
        emit toggleHotkeyChanged(text);
        applyHotkeyPlaceholder();
    });
}

void SettingsDialog::buildAppearance(QVBoxLayout *root)
{
    auto *title = makeSectionTitle(tr("外观"), this);
    anchors_.insert(QStringLiteral("appearance"), title);
    root->addWidget(title);
    root = addSectionCard(root);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);

    themeGroup_ = new QButtonGroup(this);
    themeGroup_->setExclusive(true);

    const QVector<LineTheme> themes = LineTheme::all();
    for (int i = 0; i < themes.size(); ++i) {
        const LineTheme &t = themes.at(i);
        auto *card = new ThemeCard(t.id, t.name, themePreview(t, QSize(130, 68)), this);
        card->setToolTip(t.description);
        if (t.id == store_->state.theme) card->setChecked(true);
        themeGroup_->addButton(card, i);
        grid->addWidget(card, i / 3, i % 3);   // 左右两栏后一行放三个外观
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

    // 卡片尺寸 / 挂绳不透明度 / 收起延迟：一行一项，改动立即生效并持久化
    auto addIntCombo = [this, root](const QString &text, QComboBox **member,
                                    const QVector<QPair<QString, int>> &items,
                                    int current, void (SettingsDialog::*sig)(int)){
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        auto *lab = new QLabel(text, this);
        lab->setObjectName(QStringLiteral("rowLabel"));
        row->addWidget(lab);
        row->addStretch(1);
        auto *combo = new QComboBox(this);
        for (const auto &it : items) combo->addItem(it.first, it.second);
        int idx = combo->findData(current);
        if (idx < 0 && combo->count() > 0) {
            // 旧配置里的值可能不在新档位中（如原来的 85%），选最接近的一档显示
            int best = 0;
            int bestDiff = -1;
            for (int i = 0; i < combo->count(); ++i) {
                const int d = qAbs(combo->itemData(i).toInt() - current);
                if (bestDiff < 0 || d < bestDiff) { bestDiff = d; best = i; }
            }
            idx = best;
        }
        combo->setCurrentIndex(idx);
        combo->setCursor(Qt::PointingHandCursor);
        row->addWidget(combo);
        root->addLayout(row);
        *member = combo;
        connect(combo, &QComboBox::currentIndexChanged, this,
                [this, combo, sig](int i){ emit (this->*sig)(combo->itemData(i).toInt()); });
    };
    addIntCombo(tr("卡片尺寸"), &sizeCombo_,
                {{tr("小"), 0}, {tr("中"), 1}, {tr("大"), 2}},
                store_->state.cardSize, &SettingsDialog::cardSizeChanged);
    addIntCombo(tr("挂绳不透明度"), &opacityCombo_,
                {{QStringLiteral("20%"), 20}, {QStringLiteral("40%"), 40},
                 {QStringLiteral("60%"), 60}, {QStringLiteral("80%"), 80},
                 {QStringLiteral("100%"), 100}},
                store_->state.shelfOpacity, &SettingsDialog::shelfOpacityChanged);
    addIntCombo(tr("收起延迟"), &delayCombo_,
                {{tr("300 毫秒"), 300}, {tr("800 毫秒"), 800},
                 {tr("1200 毫秒"), 1200}, {tr("1500 毫秒"), 1500}},
                store_->state.hideDelayMs, &SettingsDialog::hideDelayChanged);
}

void SettingsDialog::buildData(QVBoxLayout *root)
{
    auto *title = makeSectionTitle(tr("数据"), this);
    anchors_.insert(QStringLiteral("data"), title);
    root->addWidget(title);
    root = addSectionCard(root);

    // 每行：左侧描述 + 右侧按钮（导出行另外带状态、进度与百分比）
    auto makeRow = [this](const QString &desc) {
        auto *r = new QHBoxLayout;
        r->setContentsMargins(0, 0, 0, 0);
        r->setSpacing(8);
        auto *lab = new QLabel(desc, this);
        lab->setObjectName(QStringLiteral("rowLabel"));
        r->addWidget(lab);
        return r;
    };

    // 导出：描述 + 状态 + 弹簧 + 进度 + 百分比 + 按钮
    auto *exportRow = makeRow(tr("导出数据备份"));
    exportRow_ = exportRow;
    exportStatus_ = new QLabel(this);
    exportStatus_->setObjectName(QStringLiteral("status"));
    exportStatus_->setVisible(false);
    exportRow->addWidget(exportStatus_);
    exportRow->addStretch(1);
    exportSpacerIndex_ = exportRow->count() - 1;

    exportProgress_ = new QProgressBar(this);
    exportProgress_->setRange(0, 100);
    exportProgress_->setValue(0);
    exportProgress_->setTextVisible(false);
    exportProgress_->setFixedHeight(12);
    exportProgress_->setMinimumWidth(120);
    exportProgress_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    exportProgress_->setVisible(false);
    exportRow->addWidget(exportProgress_);

    exportPercent_ = new QLabel(tr("0%"), this);
    exportPercent_->setObjectName(QStringLiteral("percent"));
    exportPercent_->setFixedWidth(40);
    exportPercent_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    exportPercent_->setVisible(false);
    exportRow->addWidget(exportPercent_);

    exportBtn_ = new QPushButton(tr("导出"), this);
    exportBtn_->setCursor(Qt::PointingHandCursor);
    exportBtn_->setFixedHeight(28);   // 与其它设置项行高保持一致
    exportRow->addWidget(exportBtn_);
    root->addLayout(exportRow);

    // 导入
    auto *importRow = makeRow(tr("导入数据备份"));
    importRow->addStretch(1);
    auto *importBtn = new QPushButton(tr("导入"), this);
    importBtn->setCursor(Qt::PointingHandCursor);
    importBtn->setFixedHeight(28);
    importRow->addWidget(importBtn);
    root->addLayout(importRow);

    // 打开数据目录
    auto *openRow = makeRow(tr("打开数据目录"));
    openRow->addStretch(1);
    auto *openBtn = new QPushButton(tr("打开"), this);
    openBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setFixedHeight(28);
    openRow->addWidget(openBtn);
    root->addLayout(openRow);

    connect(openBtn, &QPushButton::clicked, this, [this]{
        QDesktopServices::openUrl(QUrl::fromLocalFile(store_->root));
    });

    // 导出备份：异步调用系统 PowerShell 压缩 data/，按 zip 增长估算进度
    connect(exportBtn_, &QPushButton::clicked, this, [this]{
        if (exportProc_) return;
        const QString defName = QStringLiteral("PicRope-backup-%1.zip")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
        const QString dest = QFileDialog::getSaveFileName(
            this, tr("导出数据备份"), defName, tr("Zip 压缩包 (*.zip)"));
        if (dest.isEmpty()) return;

        // 估算数据目录总字节数，作为进度分母
        qint64 total = 0;
        QDirIterator it(store_->root, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            total += it.fileInfo().size();
        }
        exportTotal_ = qMax<qint64>(total, 1);
        exportPath_ = dest;

        exportStatus_->setVisible(false);
        exportBtn_->setEnabled(false);
        exportBtn_->setText(tr("导出中…"));
        exportRow_->setStretch(exportSpacerIndex_, 0);
        exportRow_->setStretch(exportRow_->indexOf(exportProgress_), 1);
        exportProgress_->setValue(0);
        exportPercent_->setText(tr("0%"));
        exportProgress_->setVisible(true);
        exportPercent_->setVisible(true);

        exportTimer_ = new QTimer(this);
        exportTimer_->setInterval(200);
        connect(exportTimer_, &QTimer::timeout, this, [this]{
            const qint64 written = QFileInfo(exportPath_).size();
            const qint64 pct = qBound<qint64>(qint64(0), written * 100 / exportTotal_, qint64(99));
            exportProgress_->setValue(int(pct));
            exportPercent_->setText(QStringLiteral("%1%").arg(pct));
        });
        exportTimer_->start();

        exportProc_ = new QProcess(this);
        connect(exportProc_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int code, QProcess::ExitStatus st){
            exportTimer_->stop();
            exportTimer_->deleteLater();
            exportTimer_ = nullptr;
            QProcess *proc = exportProc_;
            exportProc_ = nullptr;
            const QString err = QString::fromLocal8Bit(proc->readAllStandardError());
            proc->deleteLater();

            exportBtn_->setEnabled(true);
            exportBtn_->setText(tr("导出"));
            exportRow_->setStretch(exportSpacerIndex_, 1);
            exportRow_->setStretch(exportRow_->indexOf(exportProgress_), 0);
            exportProgress_->setVisible(false);
            exportPercent_->setVisible(false);

            // 完成后只在行内短暂显示状态（成功 / 失败均 3 秒后自动消失），不弹窗
            const bool ok = (st == QProcess::NormalExit && code == 0);
            const QString msg = ok ? tr("导出完成") : tr("导出失败");
            exportStatus_->setStyleSheet(ok ? QString()
                                            : QStringLiteral("color:#C0392B;"));
            exportStatus_->setText(msg);
            exportStatus_->setToolTip(ok ? exportPath_ : err);
            exportStatus_->setVisible(true);
            QTimer::singleShot(3000, exportStatus_, [this, msg]{
                if (exportStatus_->text() == msg)
                    exportStatus_->setVisible(false);
            });
        });

        const QString ps = QStringLiteral(
            "Compress-Archive -Path '%1\\*' -DestinationPath '%2' -Force")
            .arg(QDir::toNativeSeparators(store_->root),
                 QDir::toNativeSeparators(dest));
        exportProc_->start(QStringLiteral("powershell"),
                           {QStringLiteral("-NoProfile"), QStringLiteral("-Command"), ps});
    });

    // 导入备份：解压到临时目录，定位含 state.json 的根，再合并覆盖到 data/；
    // 兼容本应用导出的包（内容直接在根）与带顶层文件夹的压缩包
    connect(importBtn, &QPushButton::clicked, this, [this]{
        const QString src = QFileDialog::getOpenFileName(
            this, tr("导入数据备份"), QString(), tr("Zip 压缩包 (*.zip)"));
        if (src.isEmpty()) return;

        QMessageBox confirm(QMessageBox::Warning, tr("导入数据备份"),
            tr("导入将用备份中的内容覆盖当前设置与数据（图片、文字笔记等），"
               "建议先导出一份当前备份。是否继续？"),
            QMessageBox::Yes | QMessageBox::No);
        confirm.setDefaultButton(QMessageBox::No);
        if (confirm.exec() != QMessageBox::Yes) return;

        const QString ps = QStringLiteral(
            "$tmp = Join-Path $env:TEMP ('PicRope_import_' + [guid]::NewGuid().ToString());"
            "Expand-Archive -Path '%1' -DestinationPath $tmp -Force;"
            "$d = $tmp;"
            "if (-not (Test-Path (Join-Path $d 'state.json'))) {"
            " $sub = Get-ChildItem -Path $tmp -Directory | Select-Object -First 1;"
            " if ($sub) { $d = $sub.FullName }"
            "};"
            "Copy-Item -Path (Join-Path $d '*') -Destination '%2' -Recurse -Force;"
            "Remove-Item -Path $tmp -Recurse -Force")
            .arg(QDir::toNativeSeparators(src),
                 QDir::toNativeSeparators(store_->root));
        QProcess proc;
        proc.start(QStringLiteral("powershell"),
                   {QStringLiteral("-NoProfile"), QStringLiteral("-Command"), ps});
        const bool ok = proc.waitForFinished(120000) && proc.exitCode() == 0;
        if (!ok) {
            QMessageBox::warning(this, tr("导入数据备份"),
                                 tr("导入失败：%1").arg(
                                     QString::fromLocal8Bit(proc.readAllStandardError())));
            return;
        }
        QMessageBox done(QMessageBox::Question, tr("导入数据备份"),
                         tr("数据已导入，需要重启图绳才能完全生效，是否立即重启？"),
                         QMessageBox::Yes | QMessageBox::No);
        done.setDefaultButton(QMessageBox::Yes);
        if (done.exec() != QMessageBox::Yes) return;
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
        QCoreApplication::quit();
    });

}

void SettingsDialog::buildOther(QVBoxLayout *root)
{
    auto *title = makeSectionTitle(tr("其他"), this);
    anchors_.insert(QStringLiteral("other"), title);
    root->addWidget(title);
    root = addSectionCard(root);

    // 与数据分区一致：左侧描述 + 右侧按钮
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    auto *lab = new QLabel(tr("恢复默认设置"), this);
    lab->setObjectName(QStringLiteral("rowLabel"));
    row->addWidget(lab);
    row->addStretch(1);
    auto *resetBtn = new QPushButton(tr("恢复默认"), this);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setFixedHeight(28);
    row->addWidget(resetBtn);
    root->addLayout(row);

    // 恢复默认设置：保留图片、文字与收集的文件，其余项还原，重启后完全生效
    connect(resetBtn, &QPushButton::clicked, this, [this]{
        QMessageBox box(QMessageBox::Warning, tr("恢复默认设置"),
                        tr("所有设置项将恢复为默认值（图片、文字笔记与收集的文件都会保留），"
                           "是否继续？"),
                        QMessageBox::Yes | QMessageBox::No);
        box.setDefaultButton(QMessageBox::No);
        if (box.exec() != QMessageBox::Yes) return;

        SavedState def;
        def.images = store_->state.images;
        def.collection = store_->state.collection;
        def.texts = store_->state.texts;
        store_->state = def;
        store_->save();

        QMessageBox done(QMessageBox::Question, tr("恢复默认设置"),
                         tr("已恢复默认设置，重启后完全生效，是否立即重启图绳？"),
                         QMessageBox::Yes | QMessageBox::No);
        done.setDefaultButton(QMessageBox::Yes);
        if (done.exec() != QMessageBox::Yes) return;
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
        QCoreApplication::quit();
    });
}

void SettingsDialog::buildAbout(QVBoxLayout *root)
{
    auto *title = makeSectionTitle(tr("关于"), this);
    anchors_.insert(QStringLiteral("about"), title);
    root->addWidget(title);
    root = addSectionCard(root);

    // 自动检查更新
    auto *autoRow = new QHBoxLayout;
    autoRow->setContentsMargins(0, 0, 0, 0);
    auto *autoLabel = new QLabel(tr("自动检查更新"), this);
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

    // 当前版本 + 检查结果 + 下载进度 + 检查更新，全部排在同一行
    auto *verRow = new QHBoxLayout;
    updateRow_ = verRow;
    verRow->setContentsMargins(0, 0, 0, 0);
    verRow->setSpacing(8);
    auto *verLabel = new QLabel(
        tr("当前版本为 %1").arg(QCoreApplication::applicationVersion()), this);
    verLabel->setObjectName(QStringLiteral("rowLabel"));
    verRow->addWidget(verLabel);

    // 检查结果提示（例如“当前已是最新版”）：紧跟在当前版本后面
    status_ = new QLabel(this);
    status_->setObjectName(QStringLiteral("status"));
    status_->setVisible(false);
    verRow->addWidget(status_);

    // 平时的弹簧：把「检查更新」按钮顶到最右；下载时让位给进度条
    verRow->addStretch(1);
    updateSpacerIndex_ = verRow->count() - 1;

    // 下载进度与百分比：显示在当前版本与「检查更新」按钮之间
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    progress_->setValue(0);
    progress_->setTextVisible(false);
    progress_->setFixedHeight(12);
    progress_->setMinimumWidth(140);
    progress_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    progress_->setVisible(false);
    verRow->addWidget(progress_);

    percent_ = new QLabel(tr("0%"), this);
    percent_->setObjectName(QStringLiteral("percent"));
    percent_->setFixedWidth(40);
    percent_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    percent_->setVisible(false);
    verRow->addWidget(percent_);

    checkBtn_ = new QPushButton(tr("检查更新"), this);
    checkBtn_->setCursor(Qt::PointingHandCursor);
    checkBtn_->setFixedHeight(28);
    connect(checkBtn_, &QPushButton::clicked, this, &SettingsDialog::startManualCheck);
    verRow->addWidget(checkBtn_);
    root->addLayout(verRow);
}

void SettingsDialog::startManualCheck()
{
    if (downloading_) return;

    status_->setVisible(false);
    checkBtn_->setEnabled(false);
    checkBtn_->setText(tr("检查中…"));
    updater_->checkLatest();
}

void SettingsDialog::beginDownload(const QUrl &url)
{
    downloading_ = true;
    status_->setVisible(false);
    // 下载时弹簧让位，进度条拉伸占满版本号与按钮之间的全部空间
    updateRow_->setStretch(updateSpacerIndex_, 0);
    updateRow_->setStretch(updateRow_->indexOf(progress_), 1);
    checkBtn_->setEnabled(false);
    checkBtn_->setText(tr("下载中…"));
    progress_->setValue(0);
    percent_->setText(tr("0%"));
    progress_->setVisible(true);
    percent_->setVisible(true);
    updater_->download(url);
}

void SettingsDialog::launchInstaller(const QString &filePath)
{
    percent_->setText(tr("100%"));
    progress_->setValue(100);
    checkBtn_->setText(tr("正在安装…"));

    if (!QProcess::startDetached(filePath)) {
        downloading_ = false;
        resetUpdateUi();
        QMessageBox::warning(this, tr("检查更新"),
                             tr("无法启动安装程序：%1").arg(filePath));
        return;
    }
    QCoreApplication::quit();
}

void SettingsDialog::resetUpdateUi()
{
    downloading_ = false;
    status_->setVisible(false);
    checkBtn_->setEnabled(true);
    checkBtn_->setText(tr("检查更新"));
    // 恢复弹簧：按钮回到最右，进度条不再占位
    updateRow_->setStretch(updateSpacerIndex_, 1);
    updateRow_->setStretch(updateRow_->indexOf(progress_), 0);
    progress_->setVisible(false);
    percent_->setVisible(false);
}
