#include "viewer.h"

#include <QCursor>
#include <QDir>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QSet>
#include <QWheelEvent>

namespace {
constexpr int kShadow = 6;
constexpr int kPad = 12;
constexpr int kBarH = 36;
constexpr int kRadius = 14;
constexpr int kNav = 36;
constexpr int kClose = 24;

bool isImageFile(const QFileInfo &fi)
{
    static const QSet<QString> exts = {
        QStringLiteral("png"),  QStringLiteral("jpg"),  QStringLiteral("jpeg"),
        QStringLiteral("bmp"),  QStringLiteral("gif"),  QStringLiteral("webp"),
        QStringLiteral("tif"),  QStringLiteral("tiff"), QStringLiteral("ico")
    };
    return exts.contains(fi.suffix().toLower());
}
}

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void ImageViewer::openFile(const QString &path)
{
    QScreen *screen = isVisible()
        ? QGuiApplication::screenAt(geometry().center())
        : QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 800);
    dpr_ = screen ? screen->devicePixelRatio() : 1.0;

    // 窗口尺寸固定：首次打开时按屏幕大小确定，之后不再随图片变化
    if (!sized_) {
        setFixedSize(qRound(avail.width() * 0.72), qRound(avail.height() * 0.76));
        sized_ = true;
    }

    loadDirectory(path);
    showCurrent();

    // 首次显示时水平居中，垂直方向整体下移一些，避免贴住屏幕顶部、被挂绳卡片遮挡
    const int centeredY = avail.y() + (avail.height() - height()) / 2;
    int y = centeredY + qRound(avail.height() * 0.06);
    y = qMin(y, avail.y() + avail.height() - height() - 12);
    const QPoint pos = isVisible()
        ? geometry().topLeft()
        : QPoint(avail.x() + (avail.width() - width()) / 2, y);
    move(pos);
    if (isMinimized()) showNormal();   // 从最小化恢复
    show();
    raise();
    activateWindow();
    setFocus();
}

void ImageViewer::loadDirectory(const QString &path)
{
    files_.clear();
    const QFileInfo fi(path);
    const QDir dir(fi.absolutePath());
    for (const QFileInfo &entry : dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        if (isImageFile(entry) || entry.absoluteFilePath() == fi.absoluteFilePath())
            files_.append(entry.absoluteFilePath());
    }

    index_ = -1;
    for (int i = 0; i < files_.size(); ++i) {
        if (QFileInfo(files_.at(i)).absoluteFilePath().compare(
                fi.absoluteFilePath(), Qt::CaseInsensitive) == 0) {
            index_ = i;
            break;
        }
    }
    if (index_ < 0) {
        files_.append(fi.absoluteFilePath());
        index_ = files_.size() - 1;
    }
}

void ImageViewer::showCurrent()
{
    if (index_ < 0 || index_ >= files_.size()) return;
    currentPath_ = files_.at(index_);
    image_ = QImage(currentPath_);

    // 每张图重新按窗口适配，缩放与平移复位
    zoom_ = 1.0;
    pan_ = QPointF();
    fit_ = 1.0;
    if (!image_.isNull()) {
        const QRectF area = imageRect();
        // 100% 时左右各让出箭头按钮的宽度，避免遮挡图片内容
        const double sideGutters = 2.0 * (kNav + 16);
        fit_ = qMin(1.0, qMin((area.width() - sideGutters) / qMax(1, image_.width()),
                              area.height() / qMax(1, image_.height())));
        fit_ = qMax(0.01, fit_);
    }
    rebuildDisplay();
    setWindowTitle(QFileInfo(currentPath_).fileName());
    update();
}

double ImageViewer::currentScale() const
{
    return fit_ * zoom_;
}

void ImageViewer::rebuildDisplay()
{
    if (image_.isNull()) {
        display_ = QPixmap();
        return;
    }
    const double s = currentScale();
    QSize target(qMax(1, qRound(image_.width() * s)),
                 qMax(1, qRound(image_.height() * s)));

    // 限制纹理尺寸，避免超大图缩放后占用过多显存
    const int maxSide = 4096;
    if (target.width() > maxSide || target.height() > maxSide)
        target.scale(maxSide, maxSide, Qt::KeepAspectRatio);

    display_ = QPixmap::fromImage(image_.scaled(
        QSize(qRound(target.width() * dpr_), qRound(target.height() * dpr_)),
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    display_.setDevicePixelRatio(dpr_);
}

void ImageViewer::clampPan()
{
    if (display_.isNull()) {
        pan_ = QPointF();
        return;
    }
    const QRectF area = imageRect();
    const double dispW = image_.width() * currentScale();
    const double dispH = image_.height() * currentScale();
    const double maxX = qMax(0.0, (dispW - area.width()) / 2.0);
    const double maxY = qMax(0.0, (dispH - area.height()) / 2.0);
    pan_.setX(qBound(-maxX, pan_.x(), maxX));
    pan_.setY(qBound(-maxY, pan_.y(), maxY));
}

void ImageViewer::step(int delta)
{
    if (files_.size() < 2) return;
    const int n = files_.size();
    index_ = ((index_ + delta) % n + n) % n;
    showCurrent();
}

QRectF ImageViewer::imageRect() const
{
    const QRectF bg(kShadow, kShadow, width() - 2 * kShadow, height() - 2 * kShadow);
    return QRectF(bg.left() + kPad, bg.top() + kPad,
                  bg.width() - 2 * kPad, bg.height() - kPad - kBarH);
}

QRectF ImageViewer::barRect() const
{
    const QRectF bg(kShadow, kShadow, width() - 2 * kShadow, height() - 2 * kShadow);
    return QRectF(bg.left(), bg.bottom() - kBarH, bg.width(), kBarH);
}

QRectF ImageViewer::prevRect() const
{
    const QRectF r = imageRect();
    return QRectF(r.left() + 12, r.center().y() - kNav / 2.0, kNav, kNav);
}

QRectF ImageViewer::nextRect() const
{
    const QRectF r = imageRect();
    return QRectF(r.right() - 12 - kNav, r.center().y() - kNav / 2.0, kNav, kNav);
}

QRectF ImageViewer::closeRect() const
{
    const QRectF bg(kShadow, kShadow, width() - 2 * kShadow, height() - 2 * kShadow);
    return QRectF(bg.right() - 12 - kClose, bg.top() + 12, kClose, kClose);
}

QRectF ImageViewer::minimizeRect() const
{
    return closeRect().translated(-kClose - 10, 0);
}

void ImageViewer::paintEvent(QPaintEvent *)
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

    // 深色底
    p.setBrush(QColor(20, 24, 31, 242));
    p.setPen(QPen(QColor(255, 255, 255, 26), 1));
    p.drawRoundedRect(bg, kRadius, kRadius);

    // 图片（居中 + 平移）
    const QRectF ir = imageRect();
    QPainterPath clip;
    clip.addRoundedRect(ir, 10, 10);
    p.save();
    p.setClipPath(clip, Qt::IntersectClip);
    if (!display_.isNull()) {
        const double dispW = image_.width() * currentScale();
        const double dispH = image_.height() * currentScale();
        QRectF target(ir.center().x() - dispW / 2.0 + pan_.x(),
                      ir.center().y() - dispH / 2.0 + pan_.y(),
                      dispW, dispH);
        p.drawPixmap(target, display_, QRectF(display_.rect()));
    } else {
        QFont f = font();
        f.setPixelSize(13);
        p.setFont(f);
        p.setPen(QColor(200, 206, 216));
        p.drawText(ir, Qt::AlignCenter, QStringLiteral("无法打开图片"));
    }
    p.restore();
    p.setPen(QPen(QColor(255, 255, 255, 22), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(ir, 10, 10);

    // 左右切换
    if (files_.size() > 1) {
        auto drawNav = [&](const QRectF &r, bool hover, bool left) {
            p.setPen(QPen(QColor(255, 255, 255, 40), 1));
            p.setBrush(hover ? QColor(48, 56, 70, 235) : QColor(30, 36, 46, 170));
            p.drawEllipse(r);
            const QPointF c = r.center();
            p.setPen(QPen(QColor(232, 236, 244), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const qreal dx = left ? 2.4 : -2.4;
            p.drawLine(QPointF(c.x() + dx, c.y() - 6), QPointF(c.x() - dx, c.y()));
            p.drawLine(QPointF(c.x() - dx, c.y()), QPointF(c.x() + dx, c.y() + 6));
        };
        drawNav(prevRect(), overPrev_, true);
        drawNav(nextRect(), overNext_, false);
    }

    // 关闭按钮
    {
        const QRectF r = closeRect();
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.setBrush(overClose_ ? QColor("#E5484D") : QColor(30, 36, 46, 190));
        p.drawEllipse(r);
        const QPointF c = r.center();
        const qreal rr = 4.6;
        p.setPen(QPen(QColor(overClose_ ? QColor(255, 255, 255) : QColor(226, 230, 236)),
                      1.7, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(c.x() - rr, c.y() - rr), QPointF(c.x() + rr, c.y() + rr));
        p.drawLine(QPointF(c.x() + rr, c.y() - rr), QPointF(c.x() - rr, c.y() + rr));
    }

    // 最小化按钮
    {
        const QRectF r = minimizeRect();
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.setBrush(overMin_ ? QColor(48, 56, 70, 235) : QColor(30, 36, 46, 190));
        p.drawEllipse(r);
        const QPointF c = r.center();
        const qreal hw = 4.8;
        p.setPen(QPen(QColor(226, 230, 236), 1.8, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(c.x() - hw, c.y() + 2.0), QPointF(c.x() + hw, c.y() + 2.0));
    }

    // 底栏
    p.save();
    QPainterPath bgClip;
    bgClip.addRoundedRect(bg, kRadius, kRadius);
    p.setClipPath(bgClip, Qt::IntersectClip);
    p.fillRect(barRect(), QColor(255, 255, 255, 10));
    p.restore();
    p.setPen(QPen(QColor(255, 255, 255, 18), 1));
    p.drawLine(QPointF(bg.left(), barRect().top()), QPointF(bg.right(), barRect().top()));

    QFont f = font();
    f.setPixelSize(12);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(226, 230, 236));
    const QString name = QFileInfo(currentPath_).fileName();
    const QRectF nameR(bg.left() + 14, barRect().top(), bg.width() - 170, kBarH);
    QFontMetrics fm(f);
    p.drawText(nameR, Qt::AlignVCenter | Qt::AlignLeft,
               fm.elidedText(name, Qt::ElideMiddle, int(nameR.width())));

    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(160, 168, 180));
    QString right;
    if (!image_.isNull())
        right = QStringLiteral("%1%").arg(qRound(zoom_ * 100));
    if (files_.size() > 1) {
        right += QStringLiteral("   %1 / %2").arg(index_ + 1).arg(files_.size());
    }
    const QRectF rightR(bg.right() - 160, barRect().top(), 146, kBarH);
    p.drawText(rightR, Qt::AlignVCenter | Qt::AlignRight, right);
}

void ImageViewer::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    if (closeRect().contains(e->pos())) { close(); return; }
    if (minimizeRect().contains(e->pos())) { showMinimized(); return; }
    if (files_.size() > 1 && prevRect().contains(e->pos())) { step(-1); return; }
    if (files_.size() > 1 && nextRect().contains(e->pos())) { step(1); return; }

    if (barRect().contains(e->pos())) {
        movingWindow_ = true;
        dragOffset_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
        return;
    }
    if (imageRect().contains(e->pos())) {
        // 未放大时整张图没有可平移的内容，按住即可拖动窗口
        if (zoom_ <= 1.0 + 0.001) {
            movingWindow_ = true;
            dragOffset_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
            return;
        }
        panning_ = true;
        dragStart_ = e->position();
        panStart_ = pan_;
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent *e)
{
    const bool np = files_.size() > 1 && prevRect().contains(e->pos());
    const bool nn = files_.size() > 1 && nextRect().contains(e->pos());
    const bool nc = closeRect().contains(e->pos());
    const bool nm = minimizeRect().contains(e->pos());
    if (np != overPrev_ || nn != overNext_ || nc != overClose_ || nm != overMin_) {
        overPrev_ = np;
        overNext_ = nn;
        overClose_ = nc;
        overMin_ = nm;
        setCursor(np || nn || nc || nm ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }

    if (movingWindow_) {
        move(e->globalPosition().toPoint() - dragOffset_);
    } else if (panning_) {
        pan_ = panStart_ + (e->position() - dragStart_);
        clampPan();
        update();
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *)
{
    movingWindow_ = false;
    panning_ = false;
}

void ImageViewer::wheelEvent(QWheelEvent *e)
{
    if (image_.isNull()) return;

    const int dy = e->angleDelta().y();
    if (dy == 0) return;

    const double oldScale = currentScale();
    const double factor = dy > 0 ? 1.15 : (1.0 / 1.15);
    const double nextZoom = qBound(1.0, zoom_ * factor, 8.0);
    if (qFuzzyCompare(nextZoom, zoom_)) return;

    // 以鼠标位置为锚点缩放：先算出光标下的图片坐标，缩放后让该点保持在光标下
    const QPointF areaCenter = imageRect().center();
    const QPointF p = e->position();
    const QPointF contentPoint = (p - areaCenter - pan_) / qMax(0.0001, oldScale);

    zoom_ = nextZoom;
    const double newScale = currentScale();
    pan_ = p - areaCenter - contentPoint * newScale;

    rebuildDisplay();
    clampPan();
    update();
}

void ImageViewer::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Escape:    close(); return;
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_PageUp:    step(-1); return;
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_PageDown:
    case Qt::Key_Space:     step(1); return;
    case Qt::Key_0:
        zoom_ = 1.0;
        pan_ = QPointF();
        rebuildDisplay();
        update();
        return;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        zoom_ = qBound(1.0, zoom_ * 1.15, 8.0);
        rebuildDisplay();
        clampPan();
        update();
        return;
    case Qt::Key_Minus:
        zoom_ = qBound(1.0, zoom_ / 1.15, 8.0);
        rebuildDisplay();
        clampPan();
        update();
        return;
    default: break;
    }
    QWidget::keyPressEvent(e);
}
