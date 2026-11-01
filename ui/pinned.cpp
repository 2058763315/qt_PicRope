#include "pinned.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QFileInfo>
#include <QFontMetrics>

#include "win32.h"

namespace {
constexpr int kPad = 10;
constexpr int kBarH = 34;
constexpr int kShadow = 5;
constexpr int kMaxSide = 1000;
constexpr int kRadius = 14;
}

PinnedImageWindow::PinnedImageWindow(const Gallery::ItemPtr &item, QWidget *parent)
    : QWidget(parent), item_(item)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    loadImage();

    const QRect wa = QGuiApplication::primaryScreen()
        ? QGuiApplication::primaryScreen()->availableGeometry()
        : QRect(0, 0, 1280, 800);
    const int origW = qMax(1, original_.width());
    const int origH = qMax(1, original_.height());
    const double boxW = qMin<double>(kMaxSide, wa.width() * 0.62);
    const double boxH = qMin<double>(kMaxSide - 100, wa.height() * 0.62);
    fit_ = qMin(1.0, qMin(boxW / origW, boxH / origH));
    zoom_ = 1.0;
    applyLayout(false);
}

void PinnedImageWindow::loadImage()
{
    original_ = QImage();
    if (item_) original_ = QImage(item_->path);
    if (original_.isNull() && item_ && !item_->image.thumbnail.isNull())
        original_ = item_->image.thumbnail.toImage();
    if (original_.isNull()) {
        original_ = QImage(320, 200, QImage::Format_ARGB32);
        original_.fill(QColor("#20242C"));
    }
}

QSize PinnedImageWindow::imageDisplaySize() const
{
    const double s = fit_ * zoom_;
    return QSize(qMax(24, qRound(original_.width() * s)),
                 qMax(24, qRound(original_.height() * s)));
}

QRectF PinnedImageWindow::imageRect() const
{
    const QSize d = imageDisplaySize();
    return QRectF(kShadow + kPad, kShadow + kBarH, d.width(), d.height());
}

QRectF PinnedImageWindow::barRect() const
{
    const QRectF bg(kShadow, kShadow, width() - 2 * kShadow, height() - 2 * kShadow);
    return QRectF(bg.left(), bg.top(), bg.width(), kBarH);
}

QRectF PinnedImageWindow::closeRect() const
{
    const QRectF bg(kShadow, kShadow, width() - 2 * kShadow, height() - 2 * kShadow);
    return QRectF(bg.right() - 7 - 26, bg.top() + 4, 26, 26);
}

QRectF PinnedImageWindow::pinRect() const
{
    return closeRect().translated(-30, 0);
}

QString PinnedImageWindow::titleText() const
{
    if (!item_) return QString();
    return QFileInfo(item_->path).fileName();
}

void PinnedImageWindow::applyLayout(bool keepCenter)
{
    const QPoint oldCenter = keepCenter ? geometry().center() : QPoint();
    const QSize d = imageDisplaySize();
    setFixedSize(d.width() + 2 * kPad + 2 * kShadow,
                 d.height() + kBarH + kPad + 2 * kShadow);

    if ((fit_ * zoom_) > 0) {
        display_ = QPixmap::fromImage(original_.scaled(
            d, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    if (keepCenter)
        move(oldCenter.x() - width() / 2, oldCenter.y() - height() / 2);
    update();
}

void PinnedImageWindow::showEvent(QShowEvent *)
{
    Win32::prepareOverlay(windowHandle());
    Win32::placeTopmost(windowHandle(), QRect(frameGeometry().topLeft(), size()));
}

void PinnedImageWindow::placeAtScreen(const QRect &screenGeom)
{
    const QPoint p(screenGeom.x() + (screenGeom.width() - width()) / 2,
                   screenGeom.y() + (screenGeom.height() - height()) / 3);
    move(p);
    Win32::placeTopmost(windowHandle(), QRect(p, size()));
}

void PinnedImageWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRectF bg(kShadow, kShadow, width() - 2 * kShadow, height() - 2 * kShadow);

    // 投影
    for (int i = 8; i >= 1; --i) {
        QColor c(10, 14, 22);
        c.setAlpha((9 - i) * 1.4 + 3);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(bg.adjusted(-i * 0.7, -i * 0.4 + 2, i * 0.7, i * 0.8 + 2),
                          kRadius + i, kRadius + i);
    }

    // 深色磨砂底
    p.setBrush(QColor(27, 31, 38, 238));
    p.setPen(QPen(QColor(255, 255, 255, 26), 1));
    p.drawRoundedRect(bg, kRadius, kRadius);

    // 图片
    QPainterPath imagePath;
    imagePath.addRoundedRect(imageRect(), 10, 10);
    p.save();
    p.setClipPath(imagePath, Qt::IntersectClip);
    if (!display_.isNull())
        p.drawPixmap(imageRect(), display_, QRectF(display_.rect()));
    p.restore();
    p.setPen(QPen(QColor(255, 255, 255, 22), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(imageRect(), 10, 10);

    // 顶栏
    p.save();
    QPainterPath barClip;
    barClip.addRoundedRect(bg, kRadius, kRadius);
    p.setClipPath(barClip, Qt::IntersectClip);
    p.fillRect(barRect(), QColor(255, 255, 255, 10));
    p.restore();
    p.setPen(QPen(QColor(255, 255, 255, 20), 1));
    p.drawLine(QPointF(bg.left(), bg.top() + kBarH),
               QPointF(bg.right(), bg.top() + kBarH));

    // 标题
    QFont f = font();
    f.setPixelSize(12);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(226, 230, 236));
    const QRectF titleR(bg.left() + 14, bg.top(), bg.width() - 100, kBarH);
    const QString title = titleText();
    QFontMetrics fm(f);
    p.drawText(titleR, Qt::AlignVCenter | Qt::AlignLeft,
               fm.elidedText(title, Qt::ElideMiddle, int(titleR.width())));

    // 图钉按钮
    {
        const QRectF r = pinRect();
        if (overPin_) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 34));
            p.drawRoundedRect(r, 6, 6);
        }
        const QPointF c = r.center();
        QPainterPath pin;
        pin.moveTo(c.x() - 4.2, c.y() - 5.0);
        pin.lineTo(c.x() + 4.2, c.y() - 5.0);
        pin.lineTo(c.x() + 1.6, c.y() + 3.2);
        pin.lineTo(c.x(), c.y() + 6.0);
        pin.lineTo(c.x() - 1.6, c.y() + 3.2);
        pin.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(230, 234, 242));
        p.drawPath(pin);
    }

    // 关闭按钮
    {
        const QRectF r = closeRect();
        if (overClose_) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#E5484D"));
            p.drawRoundedRect(r, 6, 6);
        }
        p.setPen(QPen(QColor(overClose_ ? QColor(255, 255, 255) : QColor(200, 206, 216)), 1.6));
        const QPointF c = r.center();
        const qreal rr = 5.4;
        p.drawLine(QPointF(c.x() - rr, c.y() - rr), QPointF(c.x() + rr, c.y() + rr));
        p.drawLine(QPointF(c.x() + rr, c.y() - rr), QPointF(c.x() - rr, c.y() + rr));
    }
}

void PinnedImageWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    if (pinRect().contains(e->pos())) { emit closed(); return; }
    if (closeRect().contains(e->pos())) { emit removeRequested(); return; }
    dragging_ = true;
    dragOffset_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
}

void PinnedImageWindow::mouseMoveEvent(QMouseEvent *e)
{
    const bool np = pinRect().contains(e->pos());
    const bool nc = closeRect().contains(e->pos());
    if (np != overPin_ || nc != overClose_) {
        overPin_ = np;
        overClose_ = nc;
        update();
    }
    if (dragging_)
        move(e->globalPosition().toPoint() - dragOffset_);
}

void PinnedImageWindow::mouseReleaseEvent(QMouseEvent *)
{
    dragging_ = false;
}

void PinnedImageWindow::wheelEvent(QWheelEvent *e)
{
    const QScreen *screen = QGuiApplication::screenAt(e->globalPosition().toPoint());
    const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 800);

    const double baseW = original_.width() * fit_;
    const double baseH = original_.height() * fit_;
    const double maxZoom = qMin(4.0, qMin((avail.width() - 40) / qMax(1.0, baseW),
                                          (avail.height() - 80) / qMax(1.0, baseH)));
    const double factor = e->angleDelta().y() > 0 ? 1.12 : (1.0 / 1.12);
    const double next = qBound(0.2, zoom_ * factor, qMax(0.2, maxZoom));
    if (qFuzzyCompare(next, zoom_)) return;
    zoom_ = next;
    applyLayout(true);
}

void PinnedImageWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) { emit closed(); return; }
    QWidget::keyPressEvent(e);
}
