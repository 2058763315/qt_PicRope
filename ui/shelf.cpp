#include "shelf.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QPropertyAnimation>
#include <QApplication>
#include <QWheelEvent>
#include <QStyle>
#include <QDateTime>
#include <QFontMetrics>
#include <QIcon>

#include "gallery.h"
#include "state.h"
#include "card.h"
#include "win32.h"

namespace {
constexpr int kShelfH = 214;
constexpr int kPeekMs = 2200;
constexpr int kEdgeMs = 320;
constexpr int kAwayMs = 1200;
const QColor kInk("#3C4654");

QIcon glyphIcon(const QString &kind)
{
    QPixmap pm(40, 40);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(kInk, 3.4));
    p.setBrush(Qt::NoBrush);
    if (kind == QLatin1String("basket")) {
        QPainterPath body;
        body.moveTo(8, 14);
        body.lineTo(32, 14);
        body.lineTo(28, 31);
        body.lineTo(12, 31);
        body.closeSubpath();
        p.drawPath(body);
        QPainterPath handle;
        handle.moveTo(12, 14);
        handle.quadTo(20, 3, 28, 14);
        p.drawPath(handle);
        p.drawLine(QPointF(15, 19), QPointF(16.5, 26));
        p.drawLine(QPointF(25, 19), QPointF(23.5, 26));
    } else if (kind == QLatin1String("texts")) {
        p.drawRoundedRect(QRectF(6, 10, 28, 20), 4, 4);
        p.drawLine(QPointF(7.5, 12), QPointF(20, 22));
        p.drawLine(QPointF(32.5, 12), QPointF(20, 22));
    } else {
        // 收起：绳子向上滑出，图标用尖角向上的折角
        QPainterPath chevron;
        chevron.moveTo(11, 25);
        chevron.lineTo(20, 16);
        chevron.lineTo(29, 25);
        p.drawPath(chevron);
    }
    pm.setDevicePixelRatio(2.0);
    return QIcon(pm);
}
} // namespace

// =====================================================================
//  ShelfButton — 带数字角标的圆形工具按钮
// =====================================================================
class ShelfButton : public QToolButton
{
public:
    explicit ShelfButton(QWidget *parent = nullptr) : QToolButton(parent)
    {
        setAutoRaise(true);
        setIconSize(QSize(20, 20));
        setFixedSize(38, 38);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(
            "QToolButton{background:transparent;border:none;border-radius:10px;}"
            "QToolButton:hover{background:rgba(28,38,54,0.08);}"
            "QToolButton:pressed{background:rgba(28,38,54,0.16);}");
    }

    void setBadge(int n)
    {
        if (badge_ == n) return;
        badge_ = n;
        update();
    }

protected:
    void paintEvent(QPaintEvent *e) override
    {
        QToolButton::paintEvent(e);
        if (badge_ <= 0) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QFont f = font();
        f.setPixelSize(9);
        f.setBold(true);
        p.setFont(f);
        const QString text = badge_ > 99 ? QStringLiteral("99+") : QString::number(badge_);
        const qreal w = qMax<qreal>(14, QFontMetricsF(f).horizontalAdvance(text) + 8);
        const QRectF pill(width() - w - 1, 1, w, 14);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(38, 46, 60, 235));
        p.drawRoundedRect(pill, 7, 7);
        p.setPen(QColor(255, 255, 255, 240));
        p.drawText(pill, Qt::AlignCenter, text);
    }

private:
    int badge_ = 0;
};

// =====================================================================
//  ShelfWindow
// =====================================================================
ShelfWindow::ShelfWindow(Gallery *gallery, StateStore *store, QWidget *parent)
    : QWidget(parent), gallery_(gallery), store_(store)
{
    theme_ = LineTheme::byId(store->state.theme);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(24, 14, 12, 18);
    root->setSpacing(6);

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_->setAttribute(Qt::WA_TranslucentBackground);
    scroll_->viewport()->setAutoFillBackground(false);
    scroll_->setStyleSheet(
        "QScrollArea{background:transparent;border:none;}"
        "QScrollArea > QWidget > QWidget{background:transparent;}");
    scroll_->viewport()->installEventFilter(this);
    scroll_->installEventFilter(this);

    container_ = new QWidget;
    container_->setAttribute(Qt::WA_TranslucentBackground);
    container_->setStyleSheet("background:transparent;");
    container_->installEventFilter(this);
    layout_ = new QHBoxLayout(container_);
    layout_->setContentsMargins(2, 2, 2, 2);
    layout_->setSpacing(10);
    layout_->addStretch(1);
    scroll_->setWidget(container_);
    root->addWidget(scroll_, 1);

    auto *btnCol = new QWidget(this);
    btnCol->setFixedWidth(40);
    auto *col = new QVBoxLayout(btnCol);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(6);
    col->addSpacing(34);            // 托板挂在绳子下方
    basketBtn_ = new ShelfButton(btnCol);
    basketBtn_->setIcon(glyphIcon(QStringLiteral("basket")));
    basketBtn_->setToolTip(QStringLiteral("图片篮子"));
    textsBtn_ = new ShelfButton(btnCol);
    textsBtn_->setIcon(glyphIcon(QStringLiteral("texts")));
    textsBtn_->setToolTip(QStringLiteral("文字信封"));
    hideBtn_ = new ShelfButton(btnCol);
    hideBtn_->setIcon(glyphIcon(QStringLiteral("hide")));
    hideBtn_->setToolTip(QStringLiteral("收起绳子"));
    col->addWidget(basketBtn_, 0, Qt::AlignHCenter);
    col->addWidget(textsBtn_, 0, Qt::AlignHCenter);
    col->addWidget(hideBtn_, 0, Qt::AlignHCenter);
    col->addStretch(1);
    btnCol->installEventFilter(this);
    root->addWidget(btnCol);

    connect(basketBtn_, &QToolButton::clicked, this, &ShelfWindow::basketRequested);
    connect(textsBtn_, &QToolButton::clicked, this, &ShelfWindow::textsRequested);
    connect(hideBtn_, &QToolButton::clicked, this, &ShelfWindow::slideOut);

    anim_ = new QPropertyAnimation(this, "pos", this);
    anim_->setDuration(260);
    anim_->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim_, &QPropertyAnimation::finished, this, [this]{
        if (hideAfterAnim_) hide();
    });

    rebuild();
    updateBadges();
    startEdgeDetector();
}

void ShelfWindow::startEdgeDetector()
{
    edgeTimer_ = new QTimer(this);
    edgeTimer_->setInterval(50);
    connect(edgeTimer_, &QTimer::timeout, this, &ShelfWindow::edgeTick);
    edgeTimer_->start();
}

void ShelfWindow::updateBadges()
{
    if (!basketBtn_ || !textsBtn_) return;
    const int pending = qMax(0, gallery_->collected.size() - gallery_->items.size());
    basketBtn_->setBadge(pending);
    basketBtn_->setToolTip(pending > 0
        ? QStringLiteral("图片篮子 · %1 张待挂").arg(pending)
        : QStringLiteral("图片篮子"));
    const int texts = store_->state.texts.size();
    textsBtn_->setBadge(texts);
    textsBtn_->setToolTip(texts > 0
        ? QStringLiteral("文字信封 · %1 条").arg(texts)
        : QStringLiteral("文字信封"));
}

QRect ShelfWindow::toolDockRect() const
{
    if (!basketBtn_ || !textsBtn_ || !hideBtn_) return QRect();
    QWidget *self = const_cast<ShelfWindow *>(this);
    QRect r(basketBtn_->mapTo(self, QPoint(0, 0)), basketBtn_->size());
    r = r.united(QRect(textsBtn_->mapTo(self, QPoint(0, 0)), textsBtn_->size()));
    r = r.united(QRect(hideBtn_->mapTo(self, QPoint(0, 0)), hideBtn_->size()));
    r.translate(pos());
    return r;
}

void ShelfWindow::setHoldOpen(bool hold)
{
    holdOpen_ = hold;
    if (hold) awaySince_ = -1;
}

void ShelfWindow::edgeTick()
{
    const QPoint cur = Win32::cursorPos();
    const Win32::Display disp = Win32::screenAt(cur);
    const bool down = Win32::asyncKeyDown(0x01);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    const QRect b = disp.bounds;
    const bool nearTop = cur.x() >= b.left() && cur.x() < b.right()
        && cur.y() >= b.top() && cur.y() <= b.top() + 2;

    // 靠近窗口右上角的系统标题按钮时让路，避免挡住关闭按钮
    if ((shown_ || nearTop || avoidingCaption_) && !down
        && Win32::overCaptionButtons(windowHandle())) {
        edgeSince_ = -1;
        prevDown_ = down;
        if (shown_) { avoidingCaption_ = true; slideOut(); }
        return;
    }
    if (avoidingCaption_ && !down && !shown_) {
        avoidingCaption_ = false;
        if (cur.y() >= b.top() && cur.y() < b.top() + 220
            && cur.x() >= b.left() && cur.x() < b.right()
            && !Win32::foregroundFullscreen(windowHandle())) {
            setScreenGeom(b);
            slideIn(0);
        }
    }

    if (!nearTop) suppressEdge_ = false;
    // 在顶部区域按下鼠标（例如点最大化窗口标题栏）时不展开，并立即收起
    if (nearTop && down && !prevDown_ && !overInteractive()) {
        suppressEdge_ = true;
        if (shown_) slideOut();
    }
    prevDown_ = down;

    // 鼠标在屏幕顶边停留片刻 → 展开
    if (nearTop && !down && !suppressEdge_ && !Win32::foregroundFullscreen(windowHandle())) {
        if (edgeSince_ < 0) edgeSince_ = now;
        else if (now - edgeSince_ >= kEdgeMs && (!shown_ || screenGeom_ != b)) {
            setScreenGeom(b);
            slideIn(0);
        }
    } else {
        edgeSince_ = -1;
    }

    if (shown_ && Win32::foregroundFullscreen(windowHandle()) && !overInteractive())
        slideOut();

    // 只在鼠标位于卡片/按钮上时接收点击，其余区域穿透到下层窗口
    const bool over = shown_ && overInteractive();
    const bool pass = !shown_ || !over;
    if (pass != passState_) {
        passState_ = pass;
        Win32::setMousePassthrough(windowHandle(), pass);
    }
    if (!shown_) return;

    const QPoint local = cur - pos();
    for (ShotCard *c : cards_)
        c->setHovered(over && QRect(c->mapTo(this, QPoint(0, 0)), c->size()).contains(local));

    const bool inside = geometry().contains(cur);
    if (inside) pinned_ = false;

    if (inside || pinned_ || down || dragActive_ || holdOpen_
        || QApplication::activePopupWidget() || now < peekUntil_) {
        awaySince_ = -1;
    } else if (awaySince_ < 0) {
        awaySince_ = now;
    } else if (now - awaySince_ > kAwayMs) {
        slideOut();
    }
}

bool ShelfWindow::overInteractive() const
{
    if (!isVisible()) return false;
    const QPoint local = Win32::cursorPos() - pos();
    if (!rect().contains(local)) return false;
    for (ShotCard *c : cards_) {
        if (c->isVisible()
            && QRect(c->mapTo(this, QPoint(0, 0)), c->size()).contains(local))
            return true;
    }
    for (QWidget *w : { static_cast<QWidget *>(basketBtn_),
                        static_cast<QWidget *>(textsBtn_),
                        static_cast<QWidget *>(hideBtn_) }) {
        if (w && w->isVisible()
            && QRect(w->mapTo(this, QPoint(0, 0)), w->size()).contains(local))
            return true;
    }
    return false;
}

void ShelfWindow::setScreenGeom(const QRect &geom)
{
    if (geom.isEmpty()) return;
    screenGeom_ = geom;
}

void ShelfWindow::placeOnScreen()
{
    QRect g = screenGeom_;
    if (g.isEmpty()) g = Win32::primaryScreen().bounds;
    setFixedSize(g.width(), kShelfH);
    if (!isVisible()) move(g.x(), g.top() - kShelfH - 6);
    placedGeom_ = g;
}

void ShelfWindow::animateTo(const QPoint &dest, bool hideAfter)
{
    anim_->stop();
    hideAfterAnim_ = hideAfter;
    if (!isVisible()) {
        move(dest);
        if (hideAfter) hide();
        return;
    }
    anim_->setStartValue(pos());
    anim_->setEndValue(dest);
    anim_->start();
}

void ShelfWindow::slideIn(int peekMs)
{
    if (peekMs > 0)
        peekUntil_ = QDateTime::currentMSecsSinceEpoch() + peekMs;

    QRect g = screenGeom_;
    if (g.isEmpty()) g = Win32::primaryScreen().bounds;

    if (shown_ && placedGeom_ == g) return;

    if (isVisible() && !placedGeom_.isNull() && placedGeom_ != g)
        hide();   // 换屏时直接重新滑入，避免宽度跳变

    shown_ = true;
    awaySince_ = -1;
    edgeSince_ = -1;
    placeOnScreen();
    if (!isVisible()) {
        move(g.x(), g.top() - kShelfH - 6);
        show();
    }
    animateTo(QPoint(g.x(), g.top()), false);
    Win32::prepareOverlay(windowHandle());
}

void ShelfWindow::slideOut()
{
    if (!shown_) return;
    shown_ = false;
    pinned_ = false;
    peekUntil_ = -1;
    awaySince_ = -1;
    emit retracted();
    const QRect g = placedGeom_.isEmpty() ? screenGeom_ : placedGeom_;
    animateTo(QPoint(g.x(), g.top() - kShelfH - 6), true);
}

bool ShelfWindow::toggleShelf()
{
    if (shown_) {
        slideOut();
    } else {
        const QPoint c = Win32::cursorPos();
        setScreenGeom(Win32::screenAt(c).bounds);
        hide();                       // 确保换到鼠标所在屏
        placedGeom_ = QRect();
        slideIn(0);
        pinned_ = true;
        peekUntil_ = -1;
    }
    return shown_;
}

void ShelfWindow::forceHidden()
{
    shown_ = false;
    pinned_ = false;
    peekUntil_ = -1;
    hideAfterAnim_ = false;
    anim_->stop();
    hide();
}

void ShelfWindow::rebuild()
{
    for (ShotCard *c : cards_) {
        layout_->removeWidget(c);
        c->hide();
        c->deleteLater();
    }
    cards_.clear();

    while (QLayoutItem *child = layout_->takeAt(0))
        delete child;
    layout_->addStretch(1);

    for (int i = 0; i < gallery_->items.size(); ++i) {
        ShotCard *card = makeCard(gallery_->items[i], i);
        cards_.append(card);
        layout_->insertWidget(layout_->count() - 1, card);
    }

    layout_->activate();
    QTimer::singleShot(0, this, [this]{
        if (auto *bar = scroll_->horizontalScrollBar()) bar->setValue(bar->maximum());
    });
    updateBadges();
    update();
}

ShotCard *ShelfWindow::makeCard(const Gallery::ItemPtr &item, int index)
{
    auto *card = new ShotCard(gallery_, item, index, this);
    card->setTheme(theme_);
    card->installEventFilter(this);
    CaptureItem *raw = item.data();

    connect(card, &ShotCard::copyRequested, this, [this, raw, card]{
        card->flashCopied();
        emit copyRequested(raw);
    });
    connect(card, &ShotCard::openRequested, this, [this, raw]{ emit openRequested(raw); });
    connect(card, &ShotCard::editRequested, this, [this, raw]{ emit editRequested(raw); });
    connect(card, &ShotCard::trashRequested, this, [this, raw]{ emit trashRequested(raw); });
    connect(card, &ShotCard::saveRequested, this, [this, raw]{ emit saveRequested(raw); });
    connect(card, &ShotCard::locateRequested, this, [this, raw]{ emit locateRequested(raw); });
    connect(card, &ShotCard::detachRequested, this, [this, raw]{ emit detachRequested(raw); });
    connect(card, &ShotCard::dragActive, this, [this](bool active){
        dragActive_ = active;
        if (active) awaySince_ = -1;
    });
    connect(card, &ShotCard::batchRequested, this, [this, raw](ShotCard::Batch b){
        int idx = -1;
        for (int i = 0; i < gallery_->items.size(); ++i)
            if (gallery_->items[i].data() == raw) { idx = i; break; }
        if (idx < 0) return;
        QVector<CaptureItem *> targets;
        const int n = gallery_->items.size();
        switch (b) {
        case ShotCard::BatchSelf:   targets.append(raw); break;
        case ShotCard::BatchLeft:   for (int i = 0; i < idx; ++i) targets.append(gallery_->items[i].data()); break;
        case ShotCard::BatchRight:  for (int i = idx + 1; i < n; ++i) targets.append(gallery_->items[i].data()); break;
        case ShotCard::BatchOthers: for (int i = 0; i < n; ++i) if (i != idx) targets.append(gallery_->items[i].data()); break;
        case ShotCard::BatchAll:    for (int i = 0; i < n; ++i) targets.append(gallery_->items[i].data()); break;
        }
        gallery_->removeMany(targets);
    });
    return card;
}

void ShelfWindow::setTheme(const LineTheme &t)
{
    theme_ = t;
    for (ShotCard *c : cards_) c->setTheme(t);
    update();
}

bool ShelfWindow::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(e);
        if (auto *bar = scroll_->horizontalScrollBar()) {
            const int delta = we->angleDelta().y();
            bar->setValue(bar->value() - delta);
        }
        return true;
    }
    return QWidget::eventFilter(o, e);
}

void ShelfWindow::wheelEvent(QWheelEvent *e)
{
    if (auto *bar = scroll_->horizontalScrollBar())
        bar->setValue(bar->value() - e->angleDelta().y());
}

void ShelfWindow::showEvent(QShowEvent *)
{
    Win32::prepareOverlay(windowHandle());
    // 默认让整条绳子穿透到桌面；鼠标压到卡片或按钮时 edgeTick 会立即改为可点击
    Win32::setMousePassthrough(windowHandle(), true);
    passState_ = true;
}

void ShelfWindow::resizeEvent(QResizeEvent *)
{
    update();
}

void ShelfWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 绳子：透明底，不遮挡桌面；轻微下垂更像真的挂绳
    const qreal ropeY = 30;
    const qreal x1 = 26;
    const qreal x2 = width() - 24;
    const qreal w = qMax<qreal>(2.4, theme_.ropeWidth);
    const qreal sag = qMin<qreal>(10.0, qMax<qreal>(3.0, (x2 - x1) * 0.008));

    QPainterPath rope;
    rope.moveTo(x1, ropeY);
    rope.quadTo((x1 + x2) / 2.0, ropeY + sag * 2.0, x2, ropeY);

    // 落影、绳身、顶部高光
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(16, 24, 38, 34), w + 1.8, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(rope.translated(0, 2.2));
    p.setPen(QPen(theme_.ropeColor, w, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(rope);
    p.setPen(QPen(theme_.ropeHighlight, qMax<qreal>(1.0, w / 2.4), Qt::SolidLine, Qt::RoundCap));
    p.drawPath(rope.translated(0, -0.9));

    // 左端的小钉（右端由工具托板的吊绳收尾）
    p.setPen(Qt::NoPen);
    p.setBrush(theme_.ropeColor.darker(125));
    p.drawEllipse(QPointF(x1, ropeY), 3.2, 3.2);
    p.setBrush(QColor(255, 255, 255, 110));
    p.drawEllipse(QPointF(x1 - 0.7, ropeY - 0.9), 1.2, 1.2);

    // 右侧工具托板：用一根短绳吊在绳子下方
    QRect dock;
    if (basketBtn_ && textsBtn_ && hideBtn_) {
        dock = QRect(basketBtn_->mapTo(this, QPoint(0, 0)), basketBtn_->size())
            .united(QRect(textsBtn_->mapTo(this, QPoint(0, 0)), textsBtn_->size()))
            .united(QRect(hideBtn_->mapTo(this, QPoint(0, 0)), hideBtn_->size()));
        dock.adjust(-6, -8, 6, 8);

        const qreal cx = dock.center().x();
        p.setPen(QPen(theme_.ropeColor, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(cx, ropeY + 1.0), QPointF(cx, dock.top() + 2));
        p.setPen(Qt::NoPen);
        p.setBrush(theme_.ropeColor.darker(125));
        p.drawEllipse(QPointF(cx, ropeY + 1.0), 2.8, 2.8);

        for (int i = 7; i >= 1; --i) {
            QColor c(18, 26, 42);
            c.setAlpha((8 - i) + 2);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(QRectF(dock).adjusted(-i * 0.6, -i * 0.4 + 2, i * 0.6, i * 0.7 + 2),
                              14 + i, 14 + i);
        }
        QLinearGradient dockFill(QPointF(dock.left(), dock.top()), QPointF(dock.left(), dock.bottom()));
        dockFill.setColorAt(0.0, QColor(255, 255, 255, 248));
        dockFill.setColorAt(1.0, QColor(246, 249, 253, 240));
        p.setBrush(dockFill);
        p.setPen(QPen(QColor("#E3E8F0"), 1));
        p.drawRoundedRect(QRectF(dock), 14, 14);
        p.setBrush(QColor(255, 255, 255, 130));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(dock.left() + 8, dock.top() + 2.5, dock.width() - 16, 2), 1, 1);
    }

    // 空状态提示卡片
    if (cards_.isEmpty()) {
        const QRect area = scroll_->geometry();
        const qreal cw = qMin<qreal>(372, area.width() - 40);
        const QRectF hint(area.left() + (area.width() - cw) / 2.0, 50, cw, 92);

        for (int i = 6; i >= 1; --i) {
            QColor c(18, 26, 42);
            c.setAlpha((7 - i) + 2);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(hint.adjusted(-i * 0.7, -i * 0.4 + 3, i * 0.7, i * 0.9 + 3),
                              16 + i * 0.6, 16 + i * 0.6);
        }
        p.setBrush(QColor(255, 255, 255, 246));
        p.setPen(QPen(QColor("#DFE5EE"), 1));
        p.drawRoundedRect(hint, 16, 16);

        QFont f = font();
        f.setPixelSize(15);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor("#2B3240"));
        p.drawText(QRectF(hint.left(), hint.top() + 17, hint.width(), 20),
                   Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("当前没有图片"));

        f.setPixelSize(12);
        f.setBold(false);
        p.setFont(f);
        p.setPen(QColor("#6E7886"));
        p.drawText(QRectF(hint.left(), hint.top() + 42, hint.width(), 18),
                   Qt::AlignHCenter | Qt::AlignVCenter,
                   QStringLiteral("截图或复制图片后，会自动挂到这里"));

        f.setPixelSize(11);
        p.setFont(f);
        p.setPen(QColor("#9AA4B2"));
        p.drawText(QRectF(hint.left(), hint.top() + 63, hint.width(), 18),
                   Qt::AlignHCenter | Qt::AlignVCenter,
                   QStringLiteral("Win + Shift + S 截图 · 单击图片即复制"));
    }
}
