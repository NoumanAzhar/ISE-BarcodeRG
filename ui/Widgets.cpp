#include "Widgets.h"

#include "Theme.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

void repolish(QWidget* w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

QPixmap fitPixmap(const QPixmap& src, const QSize& area)
{
    if (src.isNull() || area.width() < 1 || area.height() < 1)
        return src;

    const double fit = qMin(double(area.width()) / src.width(),
                            double(area.height()) / src.height());
    if (fit >= 1.0) {
        const int k = int(fit);
        return src.scaled(src.width() * k, src.height() * k,
                          Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    return src.scaled(area, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QFrame* makeCard(QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName("card");
    return card;
}

// ===========================================================================
// IconWidget

IconWidget::IconWidget(Kind kind, int size, QWidget* parent)
    : QWidget(parent), m_kind(kind)
{
    setFixedSize(size, size);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void IconWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const Theme::Colors& c = Theme::colors();
    const qreal w = width();

    // Rounded tile.
    QColor tile = c.accent;
    tile.setAlpha(Theme::isDark() ? 48 : 34);
    p.setPen(Qt::NoPen);
    p.setBrush(tile);
    p.drawRoundedRect(QRectF(0, 0, w, w), w * 0.26, w * 0.26);

    // Bars: alternating bar / space widths (starts and ends with a bar).
    static const int kPattern[] = { 2, 1, 1, 2, 3, 1, 1, 1, 2, 2, 1, 3, 2 };
    int total = 0;
    for (int u : kPattern)
        total += u;

    const qreal pad = w * 0.22;
    const QRectF inner(pad, pad, w - 2 * pad, w - 2 * pad);
    const qreal unit = inner.width() / total;

    qreal x = inner.left();
    bool bar = true;
    p.setBrush(c.accent);
    for (int u : kPattern) {
        if (bar)
            p.drawRect(QRectF(x, inner.top(), u * unit, inner.height()));
        x += u * unit;
        bar = !bar;
    }

    if (m_kind == Scan) {
        // Red scan line with a soft glow.
        const qreal y = inner.center().y();
        QColor glow = c.danger;
        glow.setAlpha(70);
        p.setBrush(glow);
        p.drawRoundedRect(QRectF(pad * 0.5, y - w * 0.06, w - pad, w * 0.12), w * 0.06, w * 0.06);
        p.setBrush(c.danger);
        p.drawRoundedRect(QRectF(pad * 0.5, y - w * 0.015, w - pad, w * 0.03), w * 0.015, w * 0.015);
    } else if (m_kind == Generate) {
        // "+" badge in the bottom-right corner.
        const QPointF centre(w * 0.78, w * 0.78);
        const qreal r = w * 0.19;
        p.setBrush(c.surface);
        p.drawEllipse(centre, r * 1.28, r * 1.28);
        p.setBrush(c.accent);
        p.drawEllipse(centre, r, r);
        p.setPen(QPen(Qt::white, w * 0.045, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(centre.x() - r * 0.5, centre.y()), QPointF(centre.x() + r * 0.5, centre.y()));
        p.drawLine(QPointF(centre.x(), centre.y() - r * 0.5), QPointF(centre.x(), centre.y() + r * 0.5));
    }
}

// ===========================================================================
// Banner

Banner::Banner(QWidget* parent)
    : QLabel(parent)
{
    setObjectName("banner");
    setWordWrap(true);
    setProperty("kind", QString("info"));
    hide();
}

void Banner::showMessage(const QString& text, Kind kind)
{
    QString k = "info";
    if (kind == Success)
        k = "success";
    else if (kind == Error)
        k = "error";

    setProperty("kind", k);
    repolish(this);
    setText(text);
    show();
}

void Banner::clearMessage()
{
    clear();
    hide();
}

// ===========================================================================
// PageHeader

PageHeader::PageHeader(const QString& title, const QString& subtitle, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* back = new QPushButton(QString(kArrowLeft) + "  Menu", this);
    back->setObjectName("ghost");
    back->setCursor(Qt::PointingHandCursor);
    back->setToolTip("Back to the main menu (Esc)");
    connect(back, &QPushButton::clicked, this, &PageHeader::backClicked);

    auto* backRow = new QHBoxLayout;
    backRow->setContentsMargins(0, 0, 0, 0);
    backRow->addWidget(back);
    backRow->addStretch(1);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("pageTitle");

    auto* subLabel = new QLabel(subtitle, this);
    subLabel->setObjectName("subtitle");
    subLabel->setWordWrap(true);

    layout->addLayout(backRow);
    layout->addWidget(titleLabel);
    layout->addWidget(subLabel);
}

// ===========================================================================
// ActionCard

ActionCard::ActionCard(IconWidget::Kind icon, const QString& title, const QString& text,
                       QWidget* parent)
    : QFrame(parent)
{
    setObjectName("actionCard");
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 28, 28, 24);
    layout->setSpacing(8);

    auto* iconWidget = new IconWidget(icon, 64, this);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("cardTitle");

    auto* textLabel = new QLabel(text, this);
    textLabel->setObjectName("cardText");
    textLabel->setWordWrap(true);

    auto* linkLabel = new QLabel(QString("Open  ") + kArrowRight, this);
    linkLabel->setObjectName("cardLink");

    layout->addWidget(iconWidget);
    layout->addSpacing(8);
    layout->addWidget(titleLabel);
    layout->addWidget(textLabel);
    layout->addStretch(1);
    layout->addWidget(linkLabel);

    // Let clicks on the labels reach the card itself.
    for (QLabel* l : { titleLabel, textLabel, linkLabel })
        l->setAttribute(Qt::WA_TransparentForMouseEvents);
}

void ActionCard::mousePressEvent(QMouseEvent* event)
{
    event->accept();    // so that the release event comes to us
}

void ActionCard::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())) {
        clearFocus();
        emit clicked();
    }
    QFrame::mouseReleaseEvent(event);
}

void ActionCard::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
        || event->key() == Qt::Key_Space) {
        emit clicked();
        return;
    }
    QFrame::keyPressEvent(event);
}

// ===========================================================================
// DropZone

DropZone::DropZone(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("dropZone");
    setAcceptDrops(true);
    setCursor(Qt::PointingHandCursor);
    setProperty("dragging", false);
}

void DropZone::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mime = event->mimeData();
    if (mime->hasUrls() && !mime->urls().isEmpty() && mime->urls().first().isLocalFile()) {
        setProperty("dragging", true);
        repolish(this);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void DropZone::dragLeaveEvent(QDragLeaveEvent* event)
{
    setProperty("dragging", false);
    repolish(this);
    event->accept();
}

void DropZone::dropEvent(QDropEvent* event)
{
    setProperty("dragging", false);
    repolish(this);

    const QMimeData* mime = event->mimeData();
    if (mime->hasUrls() && !mime->urls().isEmpty()) {
        event->acceptProposedAction();
        emit fileDropped(mime->urls().first().toLocalFile());
    }
}

void DropZone::mousePressEvent(QMouseEvent* event)
{
    event->accept();
}

void DropZone::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
        emit clicked();
    QFrame::mouseReleaseEvent(event);
}
