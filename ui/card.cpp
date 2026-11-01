#include "card.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QTimer>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QApplication>

namespace {
constexpr int kLongPressMs = 480;
constexpr int kFlashMs = 1200;
constexpr int kShadowLayers = 7;
}

ShotCard::ShotCard(Gallery *gallery, const Gallery::ItemPtr &item, int colorIndex, QWidget *parent)
    : QWidget(parent), gallery_(gallery), item_(item), colorIndex_(colorIndex)
{
    setFixedSize(cardW, cardH);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);

    if (gallery_) theme_ = LineTheme::byId(gallery_->store()->state.theme);

    // 与原版一致：每张卡片有细微的随机倾斜，像挂在绳上
    if (item) {
        const uint h = qHash(item->path, 0);
        tilt_ = (static_cast<int>(h & 7) - 3.5) * 0.7;
    }

    flashTimer_ = new QTimer(this);
    flashTimer_->setSingleShot(true);
    connect(flashTimer_, &QTimer::timeout, this, [this]{
        copiedFlash_ = false;
        update();
    });

    longPressTimer_ = new QTimer(this);
    longPressTimer_->setSingleShot(true);
    connect(longPressTimer_, &QTimer::timeout, this, [this]{
        if (!pressed_ || moved_ || !item_) return;
        held_ = true;
        update();
        emit editRequested();
    });

    clickTimer_ = new QTimer(this);
    clickTimer_->setSingleShot(true);
    connect(clickTimer_, &QTimer::timeout, this, [this]{
        if (item_) emit copyRequested();
    });
}

void ShotCard::setTheme(const LineTheme &t)
{
    theme_ = t;
    update();
}

QSize ShotCard::sizeHint() const { return QSize(cardW, cardH); }

void ShotCard::setHovered(bool on)
{
    if (hover_ == on) return;
    hover_ = on;
    if (!hover_) hoverClose_ = false;
    update();
}

void ShotCard::flashCopied()
{
    copiedFlash_ = true;
    flashTimer_->start(kFlashMs);
    update();
}

QSizeF ShotCard::photoSize() const
{
    if (item_ && !item_->image.thumbnail.isNull()) {
        const QSize s = item_->image.thumbnail.size();
        const double scale = qMin(142.0 / qMax(1, s.width()), 98.0 / qMax(1, s.height()));
        return QSizeF(s.width() * scale, s.height() * scale);
    }
    return QSizeF(142, 98);
}

QRectF ShotCard::surfaceRect() const
{
    const QSizeF ph = photoSize();
    const double w = ph.width() + 14.0;
    const double h = ph.height() + 14.0;
    return QRectF((cardW - w) / 2.0, 20.0, w, h);
}

QRectF ShotCard::imageRect() const
{
    const QRectF s = surfaceRect();
    return s.adjusted(7, 7, -7, -7);
}

QRectF ShotCard::clipRect() const
{
    return QRectF(cardW / 2.0 - 15, -2.0, 30, 36);
}

QRectF ShotCard::closeRect() const
{
    const QRectF s = surfaceRect();
    return QRectF(s.left() + 4, s.top() + 4, 22, 22);
}

bool ShotCard::clickOnClose(const QPoint &p) const
{
    return hover_ && closeRect().contains(p);
}

void ShotCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    p.save();
    // 绕顶部中心轻微旋转，形成悬挂感
    const qreal pivotX = cardW / 2.0;
    p.translate(pivotX, 18);
    p.rotate(tilt_);
    p.translate(-pivotX, -18);
    if (hover_) p.translate(0, -2);

    const QRectF s = surfaceRect();

    // 柔和投影
    for (int i = kShadowLayers; i >= 1; --i) {
        QColor c(20, 28, 44);
        c.setAlpha((hover_ ? 11 : 7) + (kShadowLayers - i) * 2);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(s.adjusted(-i * 0.7, -i * 0.45 + 3, i * 0.7, i * 0.9 + 3),
                          12 + i * 0.6, 12 + i * 0.6);
    }

    // 卡片主体
    QColor fill = hover_ ? QColor(255, 255, 255) : QColor(253, 253, 255);
    p.setBrush(fill);
    p.setPen(QPen(hover_ ? theme_.ropeColor.lighter(135) : QColor("#E2E7EF"), 1.2));
    p.drawRoundedRect(s, 12, 12);

    // 图片区
    const QRectF ir = imageRect();
    QPainterPath imagePath;
    imagePath.addRoundedRect(ir, 7, 7);
    p.fillPath(imagePath, QColor("#F2F4F8"));

    if (item_ && !item_->image.thumbnail.isNull()) {
        const QPixmap pix = item_->image.thumbnail;
        QSizeF target = pix.size();
        target.scale(ir.size(), Qt::KeepAspectRatio);
        QRectF dst(ir.left() + (ir.width() - target.width()) / 2.0,
                   ir.top() + (ir.height() - target.height()) / 2.0,
                   target.width(), target.height());
        p.save();
        p.setClipPath(imagePath, Qt::IntersectClip);
        p.drawPixmap(dst, pix, QRectF(pix.rect()));
        p.restore();
    } else {
        // 无预览时的占位图形
        p.setPen(QPen(QColor("#C9D0DA"), 1.4));
        p.setBrush(Qt::NoBrush);
        const QRectF ph(ir.center().x() - 17, ir.center().y() - 14, 34, 28);
        p.drawRoundedRect(ph, 5, 5);
        p.drawEllipse(QPointF(ph.left() + 9, ph.top() + 9), 3.2, 3.2);
        QPainterPath mount;
        mount.moveTo(ph.left() + 3, ph.bottom() - 5);
        mount.lineTo(ph.left() + 12, ph.top() + 12);
        mount.lineTo(ph.left() + 19, ph.bottom() - 9);
        mount.lineTo(ph.right() - 3, ph.bottom() - 5);
        p.drawPath(mount);
    }
    p.setPen(QPen(QColor(20, 28, 44, 18), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(ir, 7, 7);

    // 顶部木夹：夹头挂在绳线上，两片夹脚夹住卡片
    const QRectF cr = clipRect();
    const QColor clipColor = theme_.clipColors.isEmpty()
        ? theme_.ropeColor
        : theme_.clipColors[colorIndex_ % theme_.clipColors.size()];

    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(20, 26, 38, 60));
    p.drawRoundedRect(QRectF(cr.left() + 2, cr.top() + 3, cr.width() - 4, cr.height() - 4), 7, 7);

    // 两片夹脚（中间留缝）
    QLinearGradient legGrad(cr.topLeft(), cr.bottomLeft());
    legGrad.setColorAt(0.0, clipColor.lighter(118));
    legGrad.setColorAt(0.5, clipColor);
    legGrad.setColorAt(1.0, clipColor.darker(112));
    p.setBrush(legGrad);
    p.setPen(QPen(clipColor.darker(125), 1));
    p.drawRoundedRect(QRectF(cr.left() + 4, cr.top() + 11, 9.5, 16), 3.5, 3.5);
    p.drawRoundedRect(QRectF(cr.right() - 13.5, cr.top() + 11, 9.5, 16), 3.5, 3.5);
    p.setPen(QPen(QColor(20, 26, 38, 70), 0.8));
    p.drawLine(QPointF(cr.center().x() - 2.2, cr.top() + 12),
               QPointF(cr.center().x() - 2.2, cr.top() + 26));
    p.drawLine(QPointF(cr.center().x() + 2.2, cr.top() + 12),
               QPointF(cr.center().x() + 2.2, cr.top() + 26));

    // 夹头（挂在绳子上的圆头）
    QLinearGradient headGrad(QPointF(cr.left(), cr.top()), QPointF(cr.left(), cr.top() + 15));
    headGrad.setColorAt(0.0, clipColor.lighter(126));
    headGrad.setColorAt(0.55, clipColor.lighter(104));
    headGrad.setColorAt(1.0, clipColor.darker(108));
    p.setPen(QPen(clipColor.darker(128), 1));
    p.setBrush(headGrad);
    p.drawRoundedRect(QRectF(cr.left() + 1, cr.top() + 1, cr.width() - 2, 13), 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 120));
    p.drawRoundedRect(QRectF(cr.left() + 5, cr.top() + 3.4, 9, 2.2), 1.1, 1.1);

    // 金属弹簧圈
    QLinearGradient springGrad(QPointF(cr.left(), cr.top() + 13), QPointF(cr.right(), cr.top() + 13));
    springGrad.setColorAt(0.0, QColor("#7C8798"));
    springGrad.setColorAt(0.35, QColor("#D7DEE8"));
    springGrad.setColorAt(0.75, QColor("#9AA5B4"));
    springGrad.setColorAt(1.0, QColor("#6E7888"));
    p.setPen(QPen(QColor(40, 46, 58, 120), 0.9));
    p.setBrush(springGrad);
    p.drawRoundedRect(QRectF(cr.left() + 0.6, cr.top() + 13.5, cr.width() - 1.2, 6.5), 3.2, 3.2);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 170));
    p.drawRoundedRect(QRectF(cr.left() + 3, cr.top() + 14.6, cr.width() - 6, 1.5), 0.75, 0.75);
    p.setBrush(QColor(60, 68, 82, 180));
    p.drawEllipse(QPointF(cr.center().x(), cr.top() + 16.8), 1.5, 1.5);
    p.restore();

    // 悬停时出现的删除按钮
    if (hover_ && item_) {
        const QRectF cl = closeRect();
        QColor bg = hoverClose_ ? QColor("#E5484D") : QColor(24, 30, 40, 150);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawEllipse(cl);
        p.setPen(QPen(QColor(255, 255, 255, 235), 1.6));
        const QPointF c = cl.center();
        const qreal r = 4.2;
        p.drawLine(QPointF(c.x() - r, c.y() - r), QPointF(c.x() + r, c.y() + r));
        p.drawLine(QPointF(c.x() + r, c.y() - r), QPointF(c.x() - r, c.y() + r));
    }

    // “已复制”提示
    if (copiedFlash_) {
        const QRectF pill(cardW / 2.0 - 42, s.bottom() + 10, 84, 24);
        p.setPen(QPen(QColor("#DDE3EC"), 1));
        p.setBrush(QColor(255, 255, 255, 246));
        p.drawRoundedRect(pill, 12, 12);
        p.setPen(QColor("#20408E"));
        QFont f = font();
        f.setPixelSize(11);
        f.setBold(true);
        p.setFont(f);
        p.drawText(pill, Qt::AlignCenter, QStringLiteral("✓ 已复制"));
    }

    p.restore();
}

void ShotCard::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    pressed_ = true;
    moved_ = false;
    held_ = false;
    pressPos_ = e->pos();
    pressTime_.start();
    update();
    if (!clickOnClose(e->pos()) && item_)
        longPressTimer_->start(kLongPressMs);
}

void ShotCard::mouseMoveEvent(QMouseEvent *e)
{
    if (!pressed_ || !(e->buttons() & Qt::LeftButton)) {
        if (hover_ && item_) {
            const bool nc = clickOnClose(e->pos());
            if (nc != hoverClose_) { hoverClose_ = nc; update(); }
        }
        return;
    }

    const QPoint d = e->pos() - pressPos_;
    if (moved_ || d.manhattanLength() <= QApplication::startDragDistance()) return;

    moved_ = true;
    longPressTimer_->stop();
    if (!item_ || clickOnClose(e->pos())) return;

    auto *mime = new QMimeData;
    mime->setUrls({ QUrl::fromLocalFile(item_->path) });
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    QPixmap pix = grab();
    pix = pix.scaled(130, 130, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drag->setPixmap(pix);
    drag->setHotSpot(QPoint(pix.width() / 2, 24));
    emit dragActive(true);
    drag->exec(Qt::CopyAction);
    emit dragActive(false);
    pressed_ = false;
    update();
}

void ShotCard::mouseReleaseEvent(QMouseEvent *e)
{
    longPressTimer_->stop();
    if (e->button() != Qt::LeftButton || !pressed_) return;
    pressed_ = false;
    update();
    if (moved_) return;

    if (clickOnClose(e->pos())) {
        if (item_) emit trashRequested();
        return;
    }
    if (clipRect().contains(e->pos())) {
        if (item_) emit detachRequested();
        return;
    }
    if (held_ || pressTime_.elapsed() >= kLongPressMs) return;

    // 延迟到双击间隔后再复制，避免双击打开时先复制一次
    clickTimer_->start(qMin(320, QApplication::doubleClickInterval()));
}

void ShotCard::mouseDoubleClickEvent(QMouseEvent *)
{
    longPressTimer_->stop();
    clickTimer_->stop();
    pressed_ = false;
    if (item_) emit openRequested();
}

void ShotCard::enterEvent(QEnterEvent *)
{
    hover_ = true;
    update();
}

void ShotCard::leaveEvent(QEvent *)
{
    hover_ = false;
    hoverClose_ = false;
    longPressTimer_->stop();
    update();
}

void ShotCard::contextMenuEvent(QContextMenuEvent *e)
{
    longPressTimer_->stop();
    clickTimer_->stop();
    pressed_ = false;
    if (!item_) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu{background:#FFFFFF;border:1px solid #E1E6EE;border-radius:10px;padding:6px;}"
        "QMenu::item{padding:6px 26px 6px 14px;border-radius:6px;color:#2A303A;}"
        "QMenu::item:selected{background:#EEF3FB;color:#12233E;}"
        "QMenu::separator{height:1px;background:#EDF0F5;margin:5px 10px;}");

    QAction *copy = menu.addAction(QStringLiteral("复制图片"));
    QAction *open = menu.addAction(QStringLiteral("打开图片"));
    QAction *save = menu.addAction(QStringLiteral("保存副本…"));
    QAction *locate = menu.addAction(QStringLiteral("在文件夹中显示"));
    menu.addSeparator();
    QAction *detach = menu.addAction(QStringLiteral("贴到桌面"));
    QAction *edit = menu.addAction(QStringLiteral("编辑…"));
    menu.addSeparator();
    QAction *bSelf = menu.addAction(QStringLiteral("取下"));
    QMenu *batch = menu.addMenu(QStringLiteral("批量取下"));
    QAction *bLeft = batch->addAction(QStringLiteral("取下左侧"));
    QAction *bRight = batch->addAction(QStringLiteral("取下右侧"));
    QAction *bOthers = batch->addAction(QStringLiteral("取下其他"));
    QAction *bAll = batch->addAction(QStringLiteral("全部取下"));
    menu.addSeparator();
    QAction *trash = menu.addAction(QStringLiteral("删除（移入回收站）"));

    QAction *chosen = menu.exec(e->globalPos());
    if (!chosen) return;
    if (chosen == copy) emit copyRequested();
    else if (chosen == open) emit openRequested();
    else if (chosen == save) emit saveRequested();
    else if (chosen == locate) emit locateRequested();
    else if (chosen == detach) emit detachRequested();
    else if (chosen == edit) emit editRequested();
    else if (chosen == trash) emit trashRequested();
    else if (chosen == bSelf) emit batchRequested(BatchSelf);
    else if (chosen == bLeft) emit batchRequested(BatchLeft);
    else if (chosen == bRight) emit batchRequested(BatchRight);
    else if (chosen == bOthers) emit batchRequested(BatchOthers);
    else if (chosen == bAll) emit batchRequested(BatchAll);
}
