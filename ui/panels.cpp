#include "panels.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QListWidget>
#include <QListView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QHideEvent>
#include <QClipboard>
#include <QFontMetrics>
#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include <QFileInfo>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QtMath>

#include "gallery.h"
#include "texts.h"

namespace {
constexpr int kMargin = 14;

QString commonStyle()
{
    return QStringLiteral(
        "QWidget{font-family:'Microsoft YaHei UI','Segoe UI';color:#2B3240;}"
        "QToolButton#panelClose{color:#7B8492;font-size:26px;border:none;background:transparent;border-radius:12px;}"
        "QToolButton#panelClose:hover{background:#EEF1F6;color:#33405A;}"
        "QPushButton{background:#FFFFFF;border:1px solid #DCE2EC;border-radius:9px;padding:7px 15px;color:#33405A;}"
        "QPushButton:hover{background:#F4F7FC;border-color:#C9D3E2;}"
        "QPushButton:disabled{background:#F2F5FA;border-color:#E4E9F2;color:#AEB7C4;}"
        "QPushButton:checked{background:#E7EFFC;border-color:#B9CFEF;color:#1B3F7A;}"
        "QPushButton#primary{background:#2F6BFF;border:1px solid #2F6BFF;color:#FFFFFF;font-weight:600;}"
        "QPushButton#primary:hover{background:#245BE0;border-color:#245BE0;}"
        "QPushButton#primary:disabled{background:#CBD7EE;border-color:#CBD7EE;color:#F6F8FC;}"
        "QLineEdit{background:#FFFFFF;border:1px solid #DCE2EC;border-radius:9px;padding:7px 11px;"
        "selection-background-color:#CFE0FF;}"
        "QLineEdit:focus{border-color:#9DBBF0;}"
        "QListWidget{background:#F7F9FC;border:1px solid #E4E9F1;border-radius:12px;padding:6px;outline:none;}"
        "QListWidget::item{border-radius:9px;color:#33405A;padding:5px;}"
        "QListWidget::item:selected{background:#E7EFFC;color:#16345F;}"
        "QListWidget::item:hover{background:#EDF3FD;}"
        "QPlainTextEdit{background:#FBFCFE;border:1px solid #E4E9F1;border-radius:12px;padding:10px;"
        "color:#2B3240;selection-background-color:#CFE0FF;}"
        "QLabel#muted{color:#7D8797;}");
}

QIcon placeholderIcon()
{
    // 占位图标只画一次，避免列表刷新时重复绘制（首次打开卡顿的主要来源之一）
    static const QIcon icon = []{
        QPixmap pm(96, 96);
        pm.fill(QColor("#EDF1F8"));
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor("#C3CCDA"), 1.4));
        p.setBrush(Qt::NoBrush);
        const QRectF box(28, 30, 40, 34);
        p.drawRoundedRect(box, 6, 6);
        p.drawEllipse(QPointF(box.left() + 11, box.top() + 10), 3.6, 3.6);
        QPainterPath mount;
        mount.moveTo(box.left() + 4, box.bottom() - 6);
        mount.lineTo(box.left() + 15, box.top() + 15);
        mount.lineTo(box.left() + 23, box.bottom() - 10);
        mount.lineTo(box.right() - 4, box.bottom() - 6);
        p.drawPath(mount);
        return QIcon(pm);
    }();
    return icon;
}

QIcon iconFor(const ImageContent &img)
{
    if (!img.thumbnail.isNull()) return QIcon(img.thumbnail);
    return placeholderIcon();
}
} // namespace

// =====================================================================
//  PanelWindow
// =====================================================================
PanelWindow::PanelWindow(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setWindowTitle(title);
    setStyleSheet(commonStyle());

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(kMargin + 18, kMargin + 12, kMargin + 18, kMargin + 16);
    root->setSpacing(12);

    auto *header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    auto *titleLabel = new QLabel(title, this);
    QFont tf = titleLabel->font();
    tf.setPixelSize(15);
    tf.setBold(true);
    titleLabel->setFont(tf);
    header->addWidget(titleLabel, 0, Qt::AlignVCenter);
    header->addStretch(1);

    auto *close = new QToolButton(this);
    close->setObjectName(QStringLiteral("panelClose"));
    close->setText(QStringLiteral("×"));
    close->setFixedSize(36, 36);
    close->setCursor(Qt::PointingHandCursor);
    close->setToolTip(QStringLiteral("关闭"));
    connect(close, &QToolButton::clicked, this, &QWidget::hide);
    header->addWidget(close, 0, Qt::AlignVCenter);
    root->addLayout(header);

    content_ = new QVBoxLayout;
    content_->setContentsMargins(0, 0, 0, 0);
    content_->setSpacing(12);
    root->addLayout(content_, 1);
}

void PanelWindow::showPanel(const QRect &anchor)
{
    QScreen *s = QGuiApplication::screenAt(anchor.center());
    if (!s) s = QGuiApplication::primaryScreen();
    const QRect wa = s ? s->availableGeometry() : QRect(0, 0, 1280, 800);

    // 贴在工具托板左侧，顶部与托板对齐；超出屏幕时向内夹紧
    const int gap = 18;
    int x = anchor.left() - gap - width();
    int y = anchor.top();
    x = qMax(wa.left() + 12, x);
    y = qMax(wa.top() + 12, qMin(y, wa.bottom() - height() - 12));
    move(x, y);

    show();
    raise();
    activateWindow();
}

void PanelWindow::hideEvent(QHideEvent *e)
{
    emit hidden();
    QWidget::hideEvent(e);
}

void PanelWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF panel(kMargin, kMargin, width() - 2 * kMargin, height() - 2 * kMargin);

    for (int i = 10; i >= 1; --i) {
        QColor c(14, 20, 32);
        c.setAlpha((11 - i) + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(panel.adjusted(-i * 0.8, -i * 0.5 + 4, i * 0.8, i * 0.9 + 4),
                          18 + i * 0.7, 18 + i * 0.7);
    }
    p.setBrush(QColor(253, 254, 255, 251));
    p.setPen(QPen(QColor("#DDE3EC"), 1));
    p.drawRoundedRect(panel, 18, 18);
}

void PanelWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    if (e->position().y() <= kMargin + 46) {
        dragging_ = true;
        dragOffset_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}

void PanelWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (dragging_ && (e->buttons() & Qt::LeftButton))
        move(e->globalPosition().toPoint() - dragOffset_);
}

void PanelWindow::mouseReleaseEvent(QMouseEvent *)
{
    dragging_ = false;
}

// =====================================================================
//  BasketWindow
// =====================================================================
BasketWindow::BasketWindow(Gallery *gallery, QWidget *parent)
    : PanelWindow(QStringLiteral("图片篮子"), parent), gallery_(gallery)
{
    setFixedSize(760, 540);

    auto *tabRow = new QHBoxLayout;
    tabRow->setSpacing(6);
    pendingTab_ = new QPushButton(QStringLiteral("待挂"), this);
    allTab_ = new QPushButton(QStringLiteral("已挂"), this);
    pendingTab_->setCheckable(true);
    allTab_->setCheckable(true);
    pendingTab_->setChecked(true);
    connect(pendingTab_, &QPushButton::clicked, this, [this]{ setTab(0); });
    connect(allTab_, &QPushButton::clicked, this, [this]{ setTab(1); });
    summary_ = new QLabel(this);
    summary_->setObjectName(QStringLiteral("muted"));
    QFont sf = summary_->font();
    sf.setPixelSize(12);
    summary_->setFont(sf);
    tabRow->addWidget(pendingTab_);
    tabRow->addWidget(allTab_);
    tabRow->addStretch(1);
    tabRow->addWidget(summary_, 0, Qt::AlignVCenter);
    contentLayout()->addLayout(tabRow);

    list_ = new QListWidget(this);
    list_->setViewMode(QListView::IconMode);
    list_->setIconSize(QSize(96, 96));
    list_->setGridSize(QSize(116, 132));
    list_->setResizeMode(QListView::Adjust);
    list_->setMovement(QListView::Static);
    list_->setWordWrap(true);
    list_->setSpacing(4);
    list_->setUniformItemSizes(true);
    list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    contentLayout()->addWidget(list_, 1);

    auto *bottom = new QHBoxLayout;
    tip_ = new QLabel(this);
    tip_->setObjectName(QStringLiteral("muted"));
    QFont tf2 = tip_->font();
    tf2.setPixelSize(12);
    tip_->setFont(tf2);
    hangBtn_ = new QPushButton(QStringLiteral("挂上绳子"), this);
    hangBtn_->setObjectName(QStringLiteral("primary"));
    hangBtn_->setMinimumHeight(36);
    hangBtn_->setCursor(Qt::PointingHandCursor);
    hangBtn_->setEnabled(false);
    deleteBtn_ = new QPushButton(QStringLiteral("删除所选"), this);
    deleteBtn_->setMinimumHeight(36);
    deleteBtn_->setCursor(Qt::PointingHandCursor);
    deleteBtn_->setEnabled(false);
    bottom->addWidget(tip_, 0, Qt::AlignVCenter);
    bottom->addStretch(1);
    bottom->addWidget(deleteBtn_);
    bottom->addWidget(hangBtn_);
    contentLayout()->addLayout(bottom);

    connect(list_, &QListWidget::itemSelectionChanged, this, [this]{ updateSummary(); });
    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it){
        if (!it || !(it->flags() & Qt::ItemIsEnabled)) return;
        const QStringList paths{ it->data(Qt::UserRole).toString() };
        if (tab_ == 0) emit hangRequested(paths);
        else emit unhangRequested(paths);
    });
    connect(hangBtn_, &QPushButton::clicked, this, [this]{
        QStringList paths;
        for (QListWidgetItem *it : list_->selectedItems())
            paths.append(it->data(Qt::UserRole).toString());
        if (paths.isEmpty()) return;
        if (tab_ == 0) emit hangRequested(paths);
        else emit unhangRequested(paths);
    });
    connect(deleteBtn_, &QPushButton::clicked, this, [this]{
        QStringList paths;
        for (QListWidgetItem *it : list_->selectedItems())
            paths.append(it->data(Qt::UserRole).toString());
        if (!paths.isEmpty()) emit deleteRequested(paths);
    });
    // 隐藏时不必重建列表，打开面板时再刷新
    connect(gallery_, &Gallery::changed, this, [this]{
        if (isVisible()) refresh();
    });
    refresh();
}

void BasketWindow::setTab(int tab)
{
    if (tab_ == tab) return;
    tab_ = tab;
    pendingTab_->setChecked(tab == 0);
    allTab_->setChecked(tab == 1);
    refresh();
}

void BasketWindow::refresh()
{
    refreshing_ = true;
    list_->clear();

    int pendingCount = 0;
    int hungCount = 0;
    for (const Gallery::ItemPtr &item : gallery_->collected) {
        if (gallery_->contains(item->path)) ++hungCount;
        else ++pendingCount;
    }
    pendingTab_->setText(QStringLiteral("待挂 %1").arg(pendingCount));
    allTab_->setText(QStringLiteral("已挂 %1").arg(hungCount));

    QVector<Gallery::ItemPtr> view;
    for (int i = gallery_->collected.size() - 1; i >= 0; --i) {
        const Gallery::ItemPtr &item = gallery_->collected[i];
        const bool hung = gallery_->contains(item->path);
        if (tab_ == 0 ? hung : !hung) continue;   // 待挂只看未挂，已挂只看已挂
        view.append(item);
    }

    for (const Gallery::ItemPtr &item : view) {
        auto *row = new QListWidgetItem(iconFor(item->image),
                                        QFileInfo(item->path).fileName(), list_);
        row->setData(Qt::UserRole, item->path);
        row->setToolTip(item->path);
        row->setSizeHint(QSize(112, 128));
        row->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    }

    if (view.isEmpty()) {
        auto *row = new QListWidgetItem(placeholderIcon(),
            tab_ == 0 ? QStringLiteral("篮子里的图片都已挂上绳子")
                      : QStringLiteral("还没有已挂上绳子的图片"), list_);
        row->setFlags(Qt::NoItemFlags);
        row->setSizeHint(QSize(112, 128));
        row->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    }

    // 待挂 → 挂上绳子；已挂 → 取下绳子
    hangBtn_->setText(tab_ == 0 ? QStringLiteral("挂上绳子")
                                : QStringLiteral("取下绳子"));
    updateTip();

    refreshing_ = false;
    updateSummary();
}

void BasketWindow::updateTip()
{
    tip_->setText(tab_ == 0
        ? QStringLiteral("双击挂上绳子；鼠标拖动可框选，或 Ctrl / Shift 点击多选")
        : QStringLiteral("双击取下绳子；鼠标拖动可框选，或 Ctrl / Shift 点击多选"));
}

void BasketWindow::updateSummary()
{
    if (refreshing_) return;
    const int total = list_->count();
    const int selected = list_->selectedItems().size();
    if (selected > 0)
        summary_->setText(QStringLiteral("已选择 %1 张").arg(selected));
    else if (total <= 0 || !(list_->item(0)->flags() & Qt::ItemIsEnabled))
        summary_->setText(QStringLiteral("共 0 张"));
    else
        summary_->setText(QStringLiteral("共 %1 张").arg(total));
    if (hangBtn_) hangBtn_->setEnabled(selected > 0);
    if (deleteBtn_) deleteBtn_->setEnabled(selected > 0);
}

// =====================================================================
//  TextsWindow
// =====================================================================
TextsWindow::TextsWindow(TextCollection *texts, QWidget *parent)
    : PanelWindow(QStringLiteral("文字信封"), parent), texts_(texts)
{
    setFixedSize(760, 560);

    search_ = new QLineEdit(this);
    search_->setPlaceholderText(QStringLiteral("搜索文字…"));
    search_->setClearButtonEnabled(true);
    contentLayout()->addWidget(search_);

    auto *mid = new QHBoxLayout;
    mid->setSpacing(14);
    list_ = new QListWidget(this);
    list_->setFixedWidth(300);
    list_->setWordWrap(false);
    list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_->viewport()->installEventFilter(this);
    preview_ = new QPlainTextEdit(this);
    preview_->setReadOnly(true);
    preview_->setPlaceholderText(QStringLiteral("选择左侧文字可查看全文"));
    mid->addWidget(list_);
    mid->addWidget(preview_, 1);
    contentLayout()->addLayout(mid, 1);

    auto *bottom = new QHBoxLayout;
    status_ = new QLabel(this);
    status_->setObjectName(QStringLiteral("muted"));
    QFont sf2 = status_->font();
    sf2.setPixelSize(12);
    status_->setFont(sf2);
    hint_ = new QLabel(QStringLiteral("鼠标拖动可框选，或 Ctrl / Shift 点击多选"), this);
    hint_->setObjectName(QStringLiteral("muted"));
    hint_->setFont(sf2);
    copyBtn_ = new QPushButton(QStringLiteral("复制所选"), this);
    deleteBtn_ = new QPushButton(QStringLiteral("删除所选"), this);
    copyBtn_->setCursor(Qt::PointingHandCursor);
    deleteBtn_->setCursor(Qt::PointingHandCursor);
    copyBtn_->setEnabled(false);
    deleteBtn_->setEnabled(false);
    bottom->addWidget(status_, 0, Qt::AlignVCenter);
    bottom->addSpacing(12);
    bottom->addWidget(hint_, 0, Qt::AlignVCenter);
    bottom->addStretch(1);
    bottom->addWidget(copyBtn_);
    bottom->addWidget(deleteBtn_);
    contentLayout()->addLayout(bottom);

    connect(search_, &QLineEdit::textChanged, this, [this]{ refresh(); });
    connect(list_, &QListWidget::currentItemChanged, this, [this]{ updatePreview(); });
    connect(list_, &QListWidget::itemSelectionChanged, this, [this]{
        const int n = list_->selectedItems().size();
        copyBtn_->setEnabled(n > 0);
        deleteBtn_->setEnabled(n > 0);
    });
    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it){
        if (!it) return;
        const QString id = it->data(Qt::UserRole).toString();
        for (const TextItem &t : texts_->items()) {
            if (t.id == id) {
                QGuiApplication::clipboard()->setText(t.content);
                flashStatus(QStringLiteral("已复制到剪贴板"));
                break;
            }
        }
    });
    connect(copyBtn_, &QPushButton::clicked, this, &TextsWindow::copySelected);
    connect(deleteBtn_, &QPushButton::clicked, this, &TextsWindow::deleteSelected);
    // 隐藏时不必重建列表，打开面板时再刷新
    connect(texts_, &TextCollection::changed, this, [this]{
        if (isVisible()) refresh();
    });
    refresh();
}

void TextsWindow::refresh()
{
    refreshing_ = true;
    const QString keep = list_->currentItem()
        ? list_->currentItem()->data(Qt::UserRole).toString() : QString();
    const QString query = search_->text().trimmed();
    list_->clear();

    const QFontMetrics fm(list_->font());
    int shown = 0;
    for (const TextItem &t : texts_->items()) {
        if (!query.isEmpty() && !t.content.contains(query, Qt::CaseInsensitive)) continue;
        QString line = t.content.section(QLatin1Char('\n'), 0, 0).trimmed();
        if (line.isEmpty()) line = t.content.simplified();
        line = fm.elidedText(line, Qt::ElideRight, 240);
        const QString when = t.createdUtc.toLocalTime().toString(QStringLiteral("MM-dd HH:mm"));

        auto *row = new QListWidgetItem(QStringLiteral("%1\n%2").arg(line, when), list_);
        row->setData(Qt::UserRole, t.id);
        row->setSizeHint(QSize(280, 46));
        if (t.id == keep) list_->setCurrentItem(row);
        ++shown;
    }
    refreshing_ = false;

    if (shown == 0) {
        preview_->setPlainText(QString());
        preview_->setPlaceholderText(query.isEmpty()
            ? QStringLiteral("还没有收集到文字。复制文字后会自动出现在这里")
            : QStringLiteral("没有匹配的文字"));
        status_->setText(QStringLiteral("共 0 条"));
        copyBtn_->setEnabled(false);
        deleteBtn_->setEnabled(false);
        return;
    }

    status_->setText(QStringLiteral("共 %1 条").arg(shown));
    updatePreview();
    if (!list_->selectedItems().isEmpty()) {
        copyBtn_->setEnabled(true);
        deleteBtn_->setEnabled(true);
    }
}

void TextsWindow::updatePreview()
{
    QListWidgetItem *it = list_->currentItem();
    if (!it) {
        preview_->setPlainText(QString());
        return;
    }
    const QString id = it->data(Qt::UserRole).toString();
    for (const TextItem &t : texts_->items()) {
        if (t.id == id) { preview_->setPlainText(t.content); return; }
    }
    preview_->setPlainText(QString());
}

void TextsWindow::copySelected()
{
    QStringList parts;
    for (QListWidgetItem *it : list_->selectedItems()) {
        const QString id = it->data(Qt::UserRole).toString();
        for (const TextItem &t : texts_->items()) {
            if (t.id == id) { parts.append(t.content); break; }
        }
    }
    if (parts.isEmpty()) return;
    QGuiApplication::clipboard()->setText(parts.join(QStringLiteral("\n\n")));
    flashStatus(QStringLiteral("已复制 %1 条").arg(parts.size()));
}

void TextsWindow::deleteSelected()
{
    QVector<TextItem *> targets;
    for (QListWidgetItem *it : list_->selectedItems()) {
        const QString id = it->data(Qt::UserRole).toString();
        for (TextItem &t : texts_->items()) {
            if (t.id == id) { targets.append(&t); break; }
        }
    }
    if (targets.isEmpty()) return;
    const int n = targets.size();
    texts_->removeMany(targets);
    flashStatus(QStringLiteral("已删除 %1 条").arg(n));
}

void TextsWindow::flashStatus(const QString &text)
{
    status_->setText(text);
    QTimer::singleShot(1600, this, [this]{
        const int n = list_->count();
        status_->setText(QStringLiteral("共 %1 条").arg(n));
    });
}

// 左侧列表滚轮：固定小幅像素滚动，避免一格翻一页导致阅读断档
bool TextsWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == list_->viewport() && event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        QScrollBar *sb = list_->verticalScrollBar();
        const int step = 36;                          // 每格滚轮滚动像素
        const int degrees = we->angleDelta().y() / 8; // 单位：度
        int notches = qRound(degrees / 15.0);
        if (notches == 0) notches = degrees > 0 ? 1 : -1;
        sb->setValue(sb->value() - notches * step);
        return true;
    }
    return PanelWindow::eventFilter(watched, event);
}
