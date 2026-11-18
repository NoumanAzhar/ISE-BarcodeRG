#include "Neu.h"

#include "Theme.h"

#include <QCache>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QImage>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QStyle>

#include <algorithm>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Shadow engine

// Three box-blur passes (a good approximation of a Gaussian) on an 8-bit mask.
void blurMask(QImage& mask, int radius)
{
    if (radius < 1 || mask.isNull())
        return;

    const int w = mask.width();
    const int h = mask.height();
    const qsizetype stride = mask.bytesPerLine();
    uchar* base = mask.bits();
    const int window = 2 * radius + 1;

    auto pass = [&](bool horizontal) {
        const int length = horizontal ? w : h;
        const int lines = horizontal ? h : w;
        const qsizetype step = horizontal ? 1 : stride;
        const qsizetype next = horizontal ? stride : 1;
        std::vector<uchar> src(length);

        for (int k = 0; k < lines; ++k) {
            uchar* p = base + k * next;
            for (int i = 0; i < length; ++i)
                src[i] = p[i * step];

            int sum = 0;
            for (int i = -radius; i <= radius; ++i)
                sum += src[std::clamp(i, 0, length - 1)];
            for (int i = 0; i < length; ++i) {
                p[i * step] = uchar(sum / window);
                sum += src[std::min(i + radius + 1, length - 1)] - src[std::max(i - radius, 0)];
            }
        }
    };

    for (int n = 0; n < 3; ++n) {
        pass(true);
        pass(false);
    }
}

// Paints `color` through the mask (mask value scales the colour's alpha).
void drawTinted(QPainter& painter, const QImage& mask, const QColor& color, qreal strength = 1.0)
{
    QImage out(mask.size(), QImage::Format_ARGB32_Premultiplied);
    const int colorAlpha = qRound(color.alpha() * strength);

    for (int y = 0; y < mask.height(); ++y) {
        const uchar* m = mask.constScanLine(y);
        QRgb* o = reinterpret_cast<QRgb*>(out.scanLine(y));
        for (int x = 0; x < mask.width(); ++x) {
            const int a = m[x] * colorAlpha / 255;
            o[x] = qRgba(color.red() * a / 255, color.green() * a / 255,
                         color.blue() * a / 255, a);
        }
    }
    painter.drawImage(0, 0, out);
}

QPixmap makeSurface(const QSize& body, int radius, NeuStyle style, int depth,
                    const QColor& fillOverride, qreal dpr)
{
    const Theme::Colors& c = Theme::colors();
    auto px = [dpr](qreal v) { return qRound(v * dpr); };

    const int ext = style == NeuStyle::Raised ? 2 * depth : 0;
    QImage canvas(px(body.width() + 2 * ext), px(body.height() + 2 * ext),
                  QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);

    const QRectF bodyRect(px(ext), px(ext), px(body.width()), px(body.height()));
    const qreal r = radius * dpr;
    const int offset = std::max(1, px(depth * 0.75));
    const int blur = std::max(1, px(depth * 1.25) / 3);
    const bool custom = fillOverride.isValid();
    const QColor fill = custom ? fillOverride : (style == NeuStyle::Inset ? c.surface2 : c.surface);

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    if (style == NeuStyle::Raised) {
        // dark shadow down-right, light shadow up-left
        for (int pass = 0; pass < 2; ++pass) {
            const int sign = pass == 0 ? 1 : -1;
            QImage mask(canvas.size(), QImage::Format_Grayscale8);
            mask.fill(0);
            {
                QPainter mp(&mask);
                mp.setRenderHint(QPainter::Antialiasing);
                mp.setPen(Qt::NoPen);
                mp.setBrush(Qt::white);
                mp.drawRoundedRect(bodyRect.translated(sign * offset, sign * offset), r, r);
            }
            blurMask(mask, blur);
            drawTinted(p, mask, pass == 0 ? c.shadowDark : c.shadowLight);
        }

        // slightly convex face: lighter at the light source
        const bool dark = Theme::isDark();
        const qreal up = custom ? 0.12 : (dark ? 0.04 : 0.30);
        const qreal down = custom ? 0.12 : (dark ? 0.14 : 0.06);
        QLinearGradient face(bodyRect.topLeft(), bodyRect.bottomRight());
        face.setColorAt(0.0, Theme::blend(fill, Qt::white, up));
        face.setColorAt(1.0, Theme::blend(fill, Qt::black, down));
        p.setBrush(face);
        p.drawRoundedRect(bodyRect, r, r);
    } else {
        // Inset: a well with the same two shadows on the inside edges.
        QPainterPath clip;
        clip.addRoundedRect(bodyRect, r, r);
        p.setBrush(fill);
        p.drawPath(clip);

        for (int pass = 0; pass < 2; ++pass) {
            const int sign = pass == 0 ? 1 : -1;
            QImage mask(canvas.size(), QImage::Format_Grayscale8);
            mask.fill(255);
            {
                QPainter mp(&mask);
                mp.setRenderHint(QPainter::Antialiasing);
                mp.setPen(Qt::NoPen);
                mp.setBrush(Qt::black);
                mp.drawRoundedRect(bodyRect.translated(sign * offset, sign * offset), r, r);
            }
            blurMask(mask, blur);
            p.save();
            p.setClipPath(clip);
            drawTinted(p, mask, pass == 0 ? c.shadowDark : c.shadowLight, 0.8);
            p.restore();
        }
    }
    p.end();

    QPixmap pixmap = QPixmap::fromImage(canvas);
    pixmap.setDevicePixelRatio(dpr);
    return pixmap;
}

QColor outlineColor(NeuOutline outline)
{
    const Theme::Colors& c = Theme::colors();
    switch (outline) {
    case NeuOutline::Muted:   return Theme::withAlpha(c.muted, 170);
    case NeuOutline::Accent:  return c.accent;
    case NeuOutline::Success: return c.success;
    case NeuOutline::Danger:  return c.danger;
    case NeuOutline::None:    break;
    }
    return QColor();
}

void drawRing(QPainter& p, const QRect& body, int radius, const QColor& color,
              qreal width = 2.0, Qt::PenStyle line = Qt::SolidLine)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, width, line);
    if (line != Qt::SolidLine)
        pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal inset = width / 2.0;
    p.drawRoundedRect(QRectF(body).adjusted(inset, inset, -inset, -inset), radius, radius);
    p.restore();
}

// Small padlock centred on `c`, used by locked inputs.
void drawLock(QPainter& p, const QPointF& c, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 1.8);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    QPainterPath shackle;                          // the arc on top
    shackle.moveTo(c.x() - 4, c.y() - 1);
    shackle.lineTo(c.x() - 4, c.y() - 4);
    shackle.arcTo(QRectF(c.x() - 4, c.y() - 8, 8, 8), 180, -180);
    shackle.lineTo(c.x() + 4, c.y() - 1);
    p.drawPath(shackle);

    p.setBrush(color);                             // the body
    p.drawRoundedRect(QRectF(c.x() - 6, c.y() - 1, 12, 9), 2, 2);
    p.restore();
}

constexpr int kControlHeight = 44;
constexpr int kButtonDepth = 4;
constexpr int kButtonMargin = 2 * kButtonDepth;
constexpr int kButtonRadius = 14;

bool isKeyboardFocus(Qt::FocusReason reason)
{
    return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason
        || reason == Qt::ShortcutFocusReason;
}

} // namespace

// ===========================================================================
// Neu::paint

namespace Neu {

int margin(NeuStyle style, int depth)
{
    switch (style) {
    case NeuStyle::Raised: return 2 * depth;
    case NeuStyle::Inset:  return 2;
    case NeuStyle::Flat:   break;
    }
    return 0;
}

void paint(QPainter& painter, const QRect& body, int radius, NeuStyle style, int depth,
           qreal devicePixelRatio, const QColor& fill)
{
    if (style == NeuStyle::Flat || body.width() < 2 || body.height() < 2)
        return;

    static QCache<QString, QPixmap> cache(48 * 1024);   // cost = KB of pixels

    const QString key = QString("%1x%2|%3|%4|%5|%6|%7|%8")
        .arg(body.width()).arg(body.height()).arg(radius).arg(int(style)).arg(depth)
        .arg(Theme::isDark() ? 1 : 0).arg(devicePixelRatio).arg(fill.isValid() ? fill.rgba() : 0);

    QPixmap* pixmap = cache.object(key);
    if (!pixmap) {
        pixmap = new QPixmap(makeSurface(body.size(), radius, style, depth, fill, devicePixelRatio));
        const int cost = qMax(1, int(pixmap->width()) * pixmap->height() * 4 / 1024);
        cache.insert(key, pixmap, cost);
    }

    const int ext = style == NeuStyle::Raised ? 2 * depth : 0;
    painter.drawPixmap(body.topLeft() - QPoint(ext, ext), *pixmap);
}

} // namespace Neu

// ===========================================================================
// NeuFrame

NeuFrame::NeuFrame(NeuStyle style, int radius, QWidget* parent)
    : QFrame(parent), m_style(style), m_radius(radius)
{
    setFrameShape(QFrame::NoFrame);
    applyMargins();
}

void NeuFrame::applyMargins()
{
    const int m = m_reserved >= 0 ? m_reserved : Neu::margin(m_style, m_depth);
    setContentsMargins(m, m, m, m);
    update();
}

void NeuFrame::setNeuStyle(NeuStyle style)
{
    m_style = style;
    applyMargins();
}

void NeuFrame::setRadius(int radius)
{
    m_radius = radius;
    update();
}

void NeuFrame::setDepth(int depth)
{
    m_depth = depth;
    applyMargins();
}

void NeuFrame::setReservedMargin(int px)
{
    m_reserved = px;
    applyMargins();
}

void NeuFrame::setOutline(NeuOutline outline, Qt::PenStyle line)
{
    m_outline = outline;
    m_outlineLine = line;
    update();
}

void NeuFrame::setInteractive(bool on)
{
    m_interactive = on;
    setCursor(on ? Qt::PointingHandCursor : Qt::ArrowCursor);
    setFocusPolicy(on ? Qt::StrongFocus : Qt::NoFocus);
    setAttribute(Qt::WA_Hover, on);
}

void NeuFrame::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect body = contentsRect();
    const bool pressed = m_interactive && m_pressed;
    Neu::paint(p, body, m_radius, pressed ? NeuStyle::Inset : m_style, m_depth, devicePixelRatioF());

    if (m_interactive && underMouse() && !pressed) {
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::withAlpha(Theme::colors().text, 12));
        p.drawRoundedRect(body, m_radius, m_radius);
    }

    if (m_outline != NeuOutline::None)
        drawRing(p, body, m_radius, outlineColor(m_outline), 2.0, m_outlineLine);
    else if (m_interactive && m_keyboardFocus)
        drawRing(p, body, m_radius, Theme::colors().accent);
}

void NeuFrame::mousePressEvent(QMouseEvent* event)
{
    if (m_interactive && event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void NeuFrame::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_interactive && event->button() == Qt::LeftButton) {
        const bool wasPressed = m_pressed;
        m_pressed = false;
        update();
        if (wasPressed && rect().contains(event->position().toPoint())) {
            clearFocus();
            emit clicked();
        }
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void NeuFrame::keyPressEvent(QKeyEvent* event)
{
    if (m_interactive && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
                          || event->key() == Qt::Key_Space)) {
        emit clicked();
        return;
    }
    QFrame::keyPressEvent(event);
}

void NeuFrame::focusInEvent(QFocusEvent* event)
{
    m_keyboardFocus = isKeyboardFocus(event->reason());
    update();
    QFrame::focusInEvent(event);
}

void NeuFrame::focusOutEvent(QFocusEvent* event)
{
    m_keyboardFocus = false;
    update();
    QFrame::focusOutEvent(event);
}

void NeuFrame::enterEvent(QEnterEvent* event)
{
    if (m_interactive)
        update();
    QFrame::enterEvent(event);
}

void NeuFrame::leaveEvent(QEvent* event)
{
    m_pressed = false;
    if (m_interactive)
        update();
    QFrame::leaveEvent(event);
}

// ===========================================================================
// NeuButton

NeuButton::NeuButton(const QString& text, Variant variant, QWidget* parent)
    : QPushButton(text, parent), m_variant(variant)
{
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    setAutoDefault(false);
    setMinimumHeight(kControlHeight + 2 * kButtonMargin);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void NeuButton::setVariant(Variant variant)
{
    m_variant = variant;
    update();
}

QSize NeuButton::sizeHint() const
{
    const int iconWidth = icon().isNull() ? 0 : 28;
    const int textWidth = fontMetrics().horizontalAdvance(text());
    return QSize(2 * kButtonMargin + 44 + iconWidth + textWidth,
                 2 * kButtonMargin + kControlHeight);
}

QSize NeuButton::minimumSizeHint() const
{
    return sizeHint();
}

void NeuButton::paintEvent(QPaintEvent*)
{
    const Theme::Colors& c = Theme::colors();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect body = rect().adjusted(kButtonMargin, kButtonMargin, -kButtonMargin, -kButtonMargin);
    const bool down = isDown() && isEnabled();

    QColor fill;                     // invalid = theme surface
    QColor textColor = c.text;
    if (m_variant == Variant::Primary) {
        fill = down ? Theme::blend(c.accent, Qt::black, 0.18) : c.accent;
        textColor = c.onAccent;
    } else if (m_variant == Variant::Danger) {
        textColor = c.danger;
    }
    if (!isEnabled()) {
        p.setOpacity(0.5);
        textColor = c.muted;
    }

    Neu::paint(p, body, kButtonRadius, down ? NeuStyle::Inset : NeuStyle::Raised, kButtonDepth,
               devicePixelRatioF(), fill);

    if (isEnabled() && underMouse() && !down) {
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::withAlpha(m_variant == Variant::Primary ? Qt::white : c.text, 16));
        p.drawRoundedRect(body, kButtonRadius, kButtonRadius);
    }
    if (m_keyboardFocus && isEnabled())
        drawRing(p, body, kButtonRadius, c.accent);

    // icon + text, centred together
    QRect content = body;
    if (down)
        content.translate(0, 1);

    const int iconSize = 20;
    const int gap = 8;
    const int textWidth = fontMetrics().horizontalAdvance(text());
    const bool hasIcon = !icon().isNull();
    const int total = textWidth + (hasIcon ? iconSize + gap : 0);
    int x = content.center().x() - total / 2;

    if (hasIcon) {
        const QPixmap pm = icon().pixmap(QSize(iconSize, iconSize), devicePixelRatioF());
        p.drawPixmap(QRect(x, content.center().y() - iconSize / 2, iconSize, iconSize), pm);
        x += iconSize + gap;
    }
    p.setPen(textColor);
    p.setFont(font());
    p.drawText(QRect(x, content.top(), textWidth + 2, content.height()),
               Qt::AlignVCenter | Qt::AlignLeft, text());
}

void NeuButton::focusInEvent(QFocusEvent* event)
{
    m_keyboardFocus = isKeyboardFocus(event->reason());
    QPushButton::focusInEvent(event);
}

void NeuButton::focusOutEvent(QFocusEvent* event)
{
    m_keyboardFocus = false;
    QPushButton::focusOutEvent(event);
}

void NeuButton::enterEvent(QEnterEvent* event)
{
    update();
    QPushButton::enterEvent(event);
}

void NeuButton::leaveEvent(QEvent* event)
{
    update();
    QPushButton::leaveEvent(event);
}

// ===========================================================================
// NeuLineEdit

namespace {
constexpr int kEditMargin = 2;
constexpr int kEditRadius = 12;
}

NeuLineEdit::NeuLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setProperty("neu", true);           // style sheet: transparent, no border
    setTextMargins(14, 0, 14, 0);
    setMinimumHeight(kControlHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void NeuLineEdit::setLarge(bool large)
{
    m_large = large;
    setProperty("large", large);
    style()->unpolish(this);
    style()->polish(this);
    setMinimumHeight(large ? 64 : kControlHeight);
    updateGeometry();
}

void NeuLineEdit::setState(const QString& state)
{
    setProperty("state", state);
    update();
}

void NeuLineEdit::setLocked(bool locked)
{
    m_locked = locked;
    setReadOnly(locked);
    setFocusPolicy(locked ? Qt::NoFocus : Qt::StrongFocus);
    setCursor(locked ? Qt::ForbiddenCursor : Qt::IBeamCursor);
    setTextMargins(14, 0, locked ? 38 : 14, 0);     // room for the padlock
    setProperty("locked", locked);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

QSize NeuLineEdit::sizeHint() const
{
    return QSize(200, m_large ? 64 : kControlHeight);
}

QSize NeuLineEdit::minimumSizeHint() const
{
    return QSize(80, m_large ? 64 : kControlHeight);
}

void NeuLineEdit::paintEvent(QPaintEvent* event)
{
    {
        const Theme::Colors& c = Theme::colors();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRect body = rect().adjusted(kEditMargin, kEditMargin, -kEditMargin, -kEditMargin);
        if (m_locked)
            p.setOpacity(0.55);
        Neu::paint(p, body, kEditRadius, NeuStyle::Inset, 4, devicePixelRatioF());
        p.setOpacity(1.0);

        const QString state = property("state").toString();
        if (state == "invalid")
            drawRing(p, body, kEditRadius, c.danger);
        else if (state == "valid")
            drawRing(p, body, kEditRadius, c.success);
        else if (hasFocus())
            drawRing(p, body, kEditRadius, c.accent);

        if (m_locked) {
            drawRing(p, body, kEditRadius, Theme::withAlpha(c.muted, 150), 1.5, Qt::DashLine);
            drawLock(p, QPointF(body.right() - 20, body.center().y()), c.muted);
        }
    }
    QLineEdit::paintEvent(event);     // text, cursor (the style sheet paints no box)
}

// ===========================================================================
// NeuSegmented

namespace {
constexpr int kSegHeight = 48;
constexpr int kTrackMargin = 2;
constexpr int kPillInset = 5;
constexpr int kPillDepth = 3;
}

NeuSegmented::NeuSegmented(const QStringList& labels, QWidget* parent)
    : QWidget(parent), m_labels(labels)
{
    setFocusPolicy(Qt::TabFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize NeuSegmented::sizeHint() const
{
    int widest = 0;
    for (const QString& label : m_labels)
        widest = qMax(widest, fontMetrics().horizontalAdvance(label));
    return QSize(2 * kTrackMargin + m_labels.size() * (widest + 48), kSegHeight);
}

QSize NeuSegmented::minimumSizeHint() const
{
    return sizeHint();
}

QRect NeuSegmented::segmentRect(int index) const
{
    const QRect track = rect().adjusted(kTrackMargin, kTrackMargin, -kTrackMargin, -kTrackMargin);
    const int n = qMax(1, int(m_labels.size()));
    const int left = track.left() + track.width() * index / n;
    const int right = track.left() + track.width() * (index + 1) / n;
    return QRect(left, track.top(), right - left, track.height());
}

void NeuSegmented::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_labels.size() || index == m_current)
        return;
    m_current = index;
    update();
    emit currentChanged(index);
}

void NeuSegmented::setLocked(bool locked)
{
    m_locked = locked;
    m_keyboardFocus = false;
    setFocusPolicy(locked ? Qt::NoFocus : Qt::TabFocus);
    setCursor(locked ? Qt::ForbiddenCursor : Qt::PointingHandCursor);
    update();
}

void NeuSegmented::paintEvent(QPaintEvent*)
{
    const Theme::Colors& c = Theme::colors();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (m_locked)
        p.setOpacity(0.55);

    const QRect track = rect().adjusted(kTrackMargin, kTrackMargin, -kTrackMargin, -kTrackMargin);
    Neu::paint(p, track, 16, NeuStyle::Inset, 4, devicePixelRatioF());

    QPainterPath trackShape;
    trackShape.addRoundedRect(track, 16, 16);

    p.save();
    p.setClipPath(trackShape);
    const QRect pill = segmentRect(m_current).adjusted(kPillInset, kPillInset, -kPillInset, -kPillInset);
    Neu::paint(p, pill, 12, NeuStyle::Raised, kPillDepth, devicePixelRatioF());
    p.restore();

    if (m_keyboardFocus)
        drawRing(p, track, 16, c.accent);

    p.setFont(font());
    for (int i = 0; i < m_labels.size(); ++i) {
        QFont f = font();
        f.setBold(i == m_current);
        p.setFont(f);
        p.setPen(i == m_current ? c.accent : c.text);
        p.drawText(segmentRect(i), Qt::AlignCenter, m_labels[i]);
    }

    if (m_locked) {
        p.setOpacity(1.0);
        drawRing(p, track, 16, Theme::withAlpha(c.muted, 150), 1.5, Qt::DashLine);
        drawLock(p, QPointF(track.right() - 18, track.center().y()), c.muted);
    }
}

void NeuSegmented::mousePressEvent(QMouseEvent* event)
{
    if (m_locked) {
        event->accept();
        return;
    }
    for (int i = 0; i < m_labels.size(); ++i) {
        if (segmentRect(i).contains(event->position().toPoint())) {
            setCurrentIndex(i);
            break;
        }
    }
    event->accept();
}

void NeuSegmented::keyPressEvent(QKeyEvent* event)
{
    if (m_locked) {
        QWidget::keyPressEvent(event);
        return;
    }
    if (event->key() == Qt::Key_Left)
        setCurrentIndex(m_current - 1);
    else if (event->key() == Qt::Key_Right)
        setCurrentIndex(m_current + 1);
    else
        QWidget::keyPressEvent(event);
}

void NeuSegmented::focusInEvent(QFocusEvent* event)
{
    m_keyboardFocus = isKeyboardFocus(event->reason());
    update();
    QWidget::focusInEvent(event);
}

void NeuSegmented::focusOutEvent(QFocusEvent* event)
{
    m_keyboardFocus = false;
    update();
    QWidget::focusOutEvent(event);
}
