#include "settings.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequenceEdit>
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
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
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
        "QScrollBar::handle:vertical{background:#D2DAE8;border-radius:4px;min-height:30px;}"
        "QScrollBar::handle:vertical:hover{background:#BFC9DA;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"));

    // 内容放进滚动区：窗口高度不够时上下滑动，不再为了塞下内容拉高窗口
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("settingsScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->viewport()->setAutoFillBackground(false);
    // 设置窗口本身是透明窗口：滚动区、内容容器都必须画白底，否则会透出桌面
    scroll->setAutoFillBackground(true);
    QPalette scrollPal = scroll->palette();
    scrollPal.setColor(QPalette::Window, QColor("#FDFEFF"));
    scroll->setPalette(scrollPal);

    auto *body = new QWidget;
    body->setObjectName(QStringLiteral("settingsBody"));
    // 只对容器自身设背景，避免级联把下拉弹层也刷成透明
    body->setAutoFillBackground(true);
    QPalette bodyPal = body->palette();
    bodyPal.setColor(QPalette::Window, QColor("#FDFEFF"));
    body->setPalette(bodyPal);
    auto *root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 10, 0);   // 右侧留出滚动条位置
    root->setSpacing(12);

    buildFeatures(root);
    root->addSpacing(10);
    buildShortcuts(root);
    root->addSpacing(10);
    buildAppearance(root);
    root->addSpacing(10);
    buildData(root);
    root->addSpacing(10);
    buildAbout(root);
    root->addStretch(1);

    scroll->setWidget(body);
    contentLayout()->addWidget(scroll, 1);

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
    QScreen *s = QGuiApplication::primaryScreen();
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
}

void SettingsDialog::buildFeatures(QVBoxLayout *root)
{
    root->addWidget(makeSectionTitle(tr("功能"), this));

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
}

void SettingsDialog::buildShortcuts(QVBoxLayout *root)
{
    root->addWidget(makeSectionTitle(tr("快捷键"), this));

    // 显示/收起挂绳：默认 Ctrl+Alt+T，点进输入框按下组合键即可修改
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    auto *lab = new QLabel(tr("显示 / 收起挂绳"), this);
    lab->setObjectName(QStringLiteral("rowLabel"));
    row->addWidget(lab);
    row->addStretch(1);

    auto *edit = new QKeySequenceEdit(this);
    hotkeyEdit_ = edit;
    edit->setMaximumSequenceLength(1);
    edit->setClearButtonEnabled(false);
    edit->setFixedWidth(170);
    edit->setCursor(Qt::PointingHandCursor);
    edit->setKeySequence(QKeySequence(store_->state.toggleHotkey));
    row->addWidget(edit);
    root->addLayout(row);

    connect(edit, &QKeySequenceEdit::editingFinished, this, [this, edit]{
        const QKeySequence seq = edit->keySequence();
        auto restore = [this, edit]{
            const QSignalBlocker b(edit);
            edit->setKeySequence(QKeySequence(store_->state.toggleHotkey));
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
        if (text == store_->state.toggleHotkey) return;
        store_->state.toggleHotkey = text;
        store_->save();
        emit toggleHotkeyChanged(text);
    });
}

void SettingsDialog::buildAppearance(QVBoxLayout *root)
{
    root->addWidget(makeSectionTitle(tr("外观"), this));

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
        grid->addWidget(card, i / 4, i % 4);   // 一行四个外观
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
    root->addWidget(makeSectionTitle(tr("数据"), this));

    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(10);

    auto *backupBtn = new QPushButton(tr("导出数据备份…"), this);
    auto *openBtn   = new QPushButton(tr("打开数据目录"), this);
    auto *resetBtn  = new QPushButton(tr("恢复默认设置…"), this);
    for (QPushButton *b : {backupBtn, openBtn, resetBtn})
        b->setCursor(Qt::PointingHandCursor);
    row->addWidget(backupBtn);
    row->addWidget(openBtn);
    row->addWidget(resetBtn);
    row->addStretch(1);
    root->addLayout(row);

    connect(openBtn, &QPushButton::clicked, this, [this]{
        QDesktopServices::openUrl(QUrl::fromLocalFile(store_->root));
    });

    // 导出备份：调用系统 PowerShell 把整个 data/ 压缩成 zip，便于迁移
    connect(backupBtn, &QPushButton::clicked, this, [this]{
        const QString defName = QStringLiteral("PicRope-backup-%1.zip")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
        const QString dest = QFileDialog::getSaveFileName(
            this, tr("导出数据备份"), defName, tr("Zip 压缩包 (*.zip)"));
        if (dest.isEmpty()) return;

        const QString ps = QStringLiteral(
            "Compress-Archive -Path '%1\\*' -DestinationPath '%2' -Force")
            .arg(QDir::toNativeSeparators(store_->root),
                 QDir::toNativeSeparators(dest));
        QProcess proc;
        proc.start(QStringLiteral("powershell"),
                   {QStringLiteral("-NoProfile"), QStringLiteral("-Command"), ps});
        const bool ok = proc.waitForFinished(120000) && proc.exitCode() == 0;
        if (ok) {
            QMessageBox::information(this, tr("导出数据备份"),
                                     tr("备份已保存到：\n%1").arg(dest));
        } else {
            QMessageBox::warning(this, tr("导出数据备份"),
                                 tr("备份失败：%1").arg(
                                     QString::fromLocal8Bit(proc.readAllStandardError())));
        }
    });

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
    root->addWidget(makeSectionTitle(tr("关于"), this));

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
    root->addSpacing(6);

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
