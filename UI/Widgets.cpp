#include "Widgets.h"

#include "Theme.h"

#include <QCalendarWidget>
#include <QDateEdit>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFont>
#include <QIcon>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QTextCharFormat>
#include <QToolButton>
#include <QTimer>
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

QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

QWidget* makeField(const QString& caption, QWidget* input, QWidget* parent)
{
    auto* field = new QWidget(parent);
    auto* layout = new QVBoxLayout(field);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(makeLabel(caption, "fieldLabel", field));
    layout->addWidget(input);
    return field;
}

void setupDateEdit(QDateEdit* edit)
{
    edit->setCalendarPopup(true);
    edit->setDisplayFormat("d MMM yyyy");

    QCalendarWidget* calendar = edit->calendarWidget();
    calendar->setFirstDayOfWeek(Qt::Monday);
    calendar->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);   // M T W T F S S
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);          // no week numbers
    calendar->setGridVisible(false);

    // Qt paints Saturday and Sunday red by default: give every day the plain look.
    for (Qt::DayOfWeek day : { Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday,
                               Qt::Friday, Qt::Saturday, Qt::Sunday })
        calendar->setWeekdayTextFormat(day, QTextCharFormat());
    QTextCharFormat header;
    header.setFontWeight(QFont::DemiBold);
    calendar->setHeaderTextFormat(header);

    // The month arrows are drawn as plain text so they are readable in both themes.
    if (auto* previous = calendar->findChild<QToolButton*>("qt_calendar_prevmonth")) {
        previous->setIcon(QIcon());
        previous->setText(QString(QChar(0x2039)));
        previous->setToolButtonStyle(Qt::ToolButtonTextOnly);
    }
    if (auto* next = calendar->findChild<QToolButton*>("qt_calendar_nextmonth")) {
        next->setIcon(QIcon());
        next->setText(QString(QChar(0x203A)));
        next->setToolButtonStyle(Qt::ToolButtonTextOnly);
    }
}

NeuFrame* makeCard(QWidget* parent)
{
    return new NeuFrame(NeuStyle::Raised, 20, parent);
}

NeuFrame* makeWell(QWidget* parent)
{
    auto* well = new NeuFrame(NeuStyle::Inset, 16, parent);
    well->setDepth(5);
    return well;
}

// ===========================================================================
// IconWidget

namespace {

// Alternating bar / space widths (starts and ends with a bar).
void drawBars(QPainter& p, const QRectF& area, const QColor& color)
{
    static const int kPattern[] = { 2, 1, 1, 2, 3, 1, 1, 1, 2, 2, 1, 3, 2 };
    int total = 0;
    for (int u : kPattern)
        total += u;

    const qreal unit = area.width() / total;
    qreal x = area.left();
    bool bar = true;
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    for (int u : kPattern) {
        if (bar)
            p.drawRect(QRectF(x, area.top(), u * unit, area.height()));
        x += u * unit;
        bar = !bar;
    }
}

} // namespace

IconWidget::IconWidget(Kind kind, int size, QWidget* parent)
    : QWidget(parent), m_kind(kind)
{
    setFixedSize(size, size);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void IconWidget::setMuted(bool muted)
{
    m_muted = muted;
    update();
}

void IconWidget::paintEvent(QPaintEvent*)
{
    const Theme::Colors& c = Theme::colors();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(width() / 100.0, height() / 100.0);     // draw in a 100 x 100 box

    const QColor ink = m_muted ? c.muted : c.accent;
    QPen stroke(ink, 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    switch (m_kind) {
    case Logo:
        p.setPen(Qt::NoPen);
        p.setBrush(c.accent);
        p.drawRoundedRect(QRectF(0, 0, 100, 100), 26, 26);
        drawBars(p, QRectF(22, 22, 56, 56), c.onAccent);
        break;

    case Generate: {
        drawBars(p, QRectF(10, 20, 68, 60), ink);
        p.setPen(Qt::NoPen);
        p.setBrush(c.bg);
        p.drawEllipse(QPointF(76, 74), 22, 22);            // ring that separates the badge
        p.setBrush(ink);
        p.drawEllipse(QPointF(76, 74), 17, 17);
        p.setPen(QPen(c.onAccent, 5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(68, 74), QPointF(84, 74));
        p.drawLine(QPointF(76, 66), QPointF(76, 82));
        break;
    }

    case Scan: {
        drawBars(p, QRectF(14, 22, 72, 56), ink);
        p.setPen(Qt::NoPen);
        QColor glow = c.danger;
        glow.setAlpha(70);
        p.setBrush(glow);
        p.drawRoundedRect(QRectF(6, 44, 88, 12), 6, 6);
        p.setBrush(c.danger);
        p.drawRoundedRect(QRectF(6, 48.5, 88, 3), 1.5, 1.5);
        break;
    }

    case Cashier: {                                         // cash register
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(28, 12, 44, 24), 6, 6);    // display
        p.drawRoundedRect(QRectF(12, 42, 76, 46), 8, 8);    // body
        p.setPen(Qt::NoPen);
        p.setBrush(ink);
        for (int row = 0; row < 2; ++row)
            for (int col = 0; col < 3; ++col)
                p.drawEllipse(QPointF(34 + col * 16, 58 + row * 16), 4, 4);
        break;
    }

    case Manager: {                                         // dashboard tiles
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(14, 14, 30, 30), 7, 7);
        p.drawRoundedRect(QRectF(56, 14, 30, 30), 7, 7);
        p.drawRoundedRect(QRectF(14, 56, 30, 30), 7, 7);
        p.setPen(Qt::NoPen);
        p.setBrush(ink);
        p.drawRoundedRect(QRectF(56, 56, 30, 30), 7, 7);
        break;
    }

    case Order: {                                           // receipt
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        QPainterPath receipt;
        receipt.moveTo(24, 12);
        receipt.lineTo(76, 12);
        receipt.lineTo(76, 88);
        receipt.lineTo(67, 80);
        receipt.lineTo(58, 88);
        receipt.lineTo(50, 80);
        receipt.lineTo(42, 88);
        receipt.lineTo(33, 80);
        receipt.lineTo(24, 88);
        receipt.closeSubpath();
        p.drawPath(receipt);
        p.drawLine(QPointF(36, 34), QPointF(64, 34));
        p.drawLine(QPointF(36, 50), QPointF(64, 50));
        p.drawLine(QPointF(36, 66), QPointF(52, 66));
        break;
    }

    case Products: {                                        // box
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        QPainterPath box;
        box.moveTo(50, 10);
        box.lineTo(86, 30);
        box.lineTo(86, 70);
        box.lineTo(50, 90);
        box.lineTo(14, 70);
        box.lineTo(14, 30);
        box.closeSubpath();
        p.drawPath(box);
        p.drawLine(QPointF(50, 50), QPointF(14, 30));
        p.drawLine(QPointF(50, 50), QPointF(86, 30));
        p.drawLine(QPointF(50, 50), QPointF(50, 90));
        break;
    }

    case Camera: {
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(10, 28, 80, 56), 10, 10);
        p.drawRoundedRect(QRectF(34, 14, 26, 14), 4, 4);
        p.drawEllipse(QPointF(50, 56), 17, 17);
        break;
    }

    case Categories: {                                      // price tag
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        QPainterPath tag;
        tag.moveTo(16, 16);
        tag.lineTo(52, 16);
        tag.lineTo(88, 52);
        tag.lineTo(54, 86);
        tag.lineTo(16, 48);
        tag.closeSubpath();
        p.drawPath(tag);
        p.setPen(Qt::NoPen);
        p.setBrush(ink);
        p.drawEllipse(QPointF(34, 34), 6, 6);
        break;
    }

    case Inventory: {                                       // shelves with boxes
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(12, 12, 76, 76), 10, 10);
        p.drawLine(QPointF(12, 50), QPointF(88, 50));
        p.setPen(Qt::NoPen);
        p.setBrush(ink);
        p.drawRoundedRect(QRectF(24, 26, 20, 16), 3, 3);
        p.drawRoundedRect(QRectF(54, 26, 20, 16), 3, 3);
        p.drawRoundedRect(QRectF(38, 62, 24, 16), 3, 3);
        break;
    }

    case Discount: {                                        // percent badge
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(50, 50), 38, 38);
        p.drawLine(QPointF(36, 66), QPointF(64, 34));
        p.setPen(Qt::NoPen);
        p.setBrush(ink);
        p.drawEllipse(QPointF(37, 37), 6, 6);
        p.drawEllipse(QPointF(63, 63), 6, 6);
        break;
    }

    case Reports: {                                         // line chart
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        QPainterPath axes;
        axes.moveTo(14, 12);
        axes.lineTo(14, 86);
        axes.lineTo(88, 86);
        p.drawPath(axes);
        QPainterPath line;
        line.moveTo(28, 66);
        line.lineTo(46, 46);
        line.lineTo(60, 58);
        line.lineTo(84, 24);
        p.drawPath(line);
        break;
    }

    case Orders: {                                          // clipboard with lines
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(20, 16, 60, 74), 8, 8);
        p.drawRoundedRect(QRectF(36, 8, 28, 16), 5, 5);
        p.drawLine(QPointF(34, 44), QPointF(66, 44));
        p.drawLine(QPointF(34, 60), QPointF(66, 60));
        p.drawLine(QPointF(34, 76), QPointF(52, 76));
        break;
    }

    case Hold: {                                            // pause
        p.setPen(stroke);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(50, 50), 38, 38);
        p.setPen(Qt::NoPen);
        p.setBrush(ink);
        p.drawRoundedRect(QRectF(36, 32, 9, 36), 3, 3);
        p.drawRoundedRect(QRectF(55, 32, 9, 36), 3, 3);
        break;
    }
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
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &Banner::clearMessage);
    hide();
}

void Banner::showMessage(const QString& text, Kind kind, int timeoutMs)
{
    QString name = "info";
    QString symbol = "i";
    switch (kind) {
    case Success: name = "success"; symbol = QString(kCheck); break;
    case Warning: name = "warning"; symbol = "!"; break;
    case Error:   name = "error";   symbol = QString(kCross); break;
    case Info:    break;
    }

    setProperty("kind", name);
    repolish(this);
    setText(symbol + "   " + text);
    show();

    if (timeoutMs > 0)
        m_timer->start(timeoutMs);
    else
        m_timer->stop();
}

void Banner::clearMessage()
{
    if (m_timer)
        m_timer->stop();
    clear();
    hide();
}

// ===========================================================================
// PageHeader

PageHeader::PageHeader(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_title = makeLabel(title, "pageTitle", this);
    layout->addWidget(m_title);
}

void PageHeader::setTitle(const QString& title)
{
    m_title->setText(title);
}

// ===========================================================================
// ActionCard

ActionCard::ActionCard(IconWidget::Kind icon, const QString& title, const QString& text,
                       QWidget* parent)
    : NeuFrame(NeuStyle::Raised, 26, parent)
{
    setDepth(8);
    setInteractive(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 26, 28, 26);
    layout->setSpacing(6);

    auto* iconWidget = new IconWidget(icon, 72, this);
    auto* titleLabel = makeLabel(title, "cardTitle", this);
    auto* textLabel = makeLabel(text, "cardText", this);
    textLabel->setWordWrap(true);

    layout->addWidget(iconWidget);
    layout->addSpacing(14);
    layout->addWidget(titleLabel);
    layout->addWidget(textLabel);
    layout->addStretch(1);

    // Let clicks on the labels reach the card itself.
    for (QLabel* l : { titleLabel, textLabel })
        l->setAttribute(Qt::WA_TransparentForMouseEvents);
}

// ===========================================================================
// DropZone

DropZone::DropZone(QWidget* parent)
    : NeuFrame(NeuStyle::Inset, 20, parent)
{
    setDepth(5);
    setAcceptDrops(true);
    setCursor(Qt::PointingHandCursor);
    setOutline(NeuOutline::Muted, Qt::DashLine);

    // Inset surface: clicking is handled here, without the pressed look.
    setInteractive(true);
    setNeuStyle(NeuStyle::Inset);
}

void DropZone::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mime = event->mimeData();
    if (mime->hasUrls() && !mime->urls().isEmpty() && mime->urls().first().isLocalFile()) {
        setOutline(NeuOutline::Accent, Qt::DashLine);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void DropZone::dragLeaveEvent(QDragLeaveEvent* event)
{
    setOutline(NeuOutline::Muted, Qt::DashLine);
    event->accept();
}

void DropZone::dropEvent(QDropEvent* event)
{
    setOutline(NeuOutline::Muted, Qt::DashLine);

    const QMimeData* mime = event->mimeData();
    if (mime->hasUrls() && !mime->urls().isEmpty()) {
        event->acceptProposedAction();
        emit fileDropped(mime->urls().first().toLocalFile());
    }
}
