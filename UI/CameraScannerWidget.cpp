#include "CameraScannerWidget.h"

#include "Neu.h"
#include "Theme.h"
#include "Widgets.h"
#include "barcode_reader.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>

#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
namespace {

constexpr int kTickMs = 30;
constexpr qint64 kSameCodeGapMs = 700;    // out of the picture at least this long = a new scan
constexpr qint64 kRepeatMs = 2000;        // a held code counts again after this long
constexpr qint64 kFlashMs = 600;          // how long the green "read" box stays
constexpr int kMissesBeforeForget = 4;
constexpr int kBeepHz = 2800;             // classic scanner pitch
constexpr int kBeepMs = 110;              // one short beep

// The classic one-tone "scanner beep". Played on its own thread so the live picture never stalls.
void playScanBeep()
{
#ifdef _WIN32
    std::thread([] { Beep(kBeepHz, kBeepMs); }).detach();
#else
    QApplication::beep();
#endif
}

QRect toQRect(const cv::Rect& r)
{
    return QRect(r.x, r.y, r.width, r.height);
}

// Frame around the barcode on the live picture.
void drawBox(QPixmap& pm, const QRect& box, const QColor& color)
{
    if (box.isEmpty())
        return;
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, qMax(3, pm.width() / 320)));
    p.setBrush(Qt::NoBrush);
    p.drawRect(box.adjusted(-4, -4, 4, 4));
}

} // namespace

CameraScannerWidget::CameraScannerWidget(QWidget* parent)
    : QWidget(parent)
{
    m_clock.start();
    m_lastEmitTime = -kRepeatMs;

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    // ---- the view: placeholder or live picture, inside an inset well ----
    m_well = makeWell(this);
    m_well->setMinimumSize(336, 256);
    m_well->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_stack = new QStackedLayout(m_well);
    m_stack->setContentsMargins(6, 6, 6, 6);

    auto* placeholder = new QWidget(m_well);
    auto* placeholderLayout = new QVBoxLayout(placeholder);
    placeholderLayout->setContentsMargins(16, 16, 16, 16);
    placeholderLayout->setSpacing(10);
    auto* icon = new IconWidget(IconWidget::Camera, 56, placeholder);
    icon->setMuted(true);
    m_placeholderText = makeLabel(QString(), "muted", placeholder);
    m_placeholderText->setAlignment(Qt::AlignCenter);
    m_placeholderText->setWordWrap(true);
    placeholderLayout->addStretch(1);
    placeholderLayout->addWidget(icon, 0, Qt::AlignHCenter);
    placeholderLayout->addWidget(m_placeholderText);
    placeholderLayout->addStretch(1);
    m_stack->addWidget(placeholder);

    m_view = new QLabel(m_well);
    m_view->setAlignment(Qt::AlignCenter);
    m_view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);   // the picture never sets the size
    m_view->setMinimumSize(1, 1);
    m_view->installEventFilter(this);                                     // re-scale on resize
    m_stack->addWidget(m_view);

    root->addWidget(m_well, 1);

    // ---- start / stop ----
    m_toggle = new NeuButton("Start camera", NeuButton::Variant::Primary, this);
    auto* buttonRow = new QHBoxLayout;
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_toggle);
    buttonRow->addStretch(1);
    root->addLayout(buttonRow);

    m_timer = new QTimer(this);
    m_timer->setInterval(kTickMs);
    connect(m_timer, &QTimer::timeout, this, &CameraScannerWidget::onTick);
    connect(m_toggle, &QPushButton::clicked, this, [this]() { setActive(!isActive()); });

    showPlaceholder("Camera is off");
}

CameraScannerWidget::~CameraScannerWidget() = default;   // here, so CameraReader is a complete type

void CameraScannerWidget::setActive(bool on)
{
    if (on == isActive())
        return;

    if (on) {
        resetScanState();
        m_reader = std::make_unique<CameraReader>();
        m_reader->start(0);                           // opens the camera in the background
        m_timer->start();
        showPlaceholder("Starting the camera" + QString(kEllipsis));
        m_toggle->setText("Stop camera");
        m_toggle->setVariant(NeuButton::Variant::Normal);
    } else {
        m_timer->stop();
        m_reader.reset();
        m_frame = QPixmap();
        m_view->clear();
        showPlaceholder("Camera is off");
        m_toggle->setText("Start camera");
        m_toggle->setVariant(NeuButton::Variant::Primary);
    }
    emit activeChanged(on);
}

void CameraScannerWidget::resetScanState()
{
    m_lastScanId = 0;
    m_box = QRect();
    m_readBox = QRect();
    m_pending.clear();
    m_pendingHits = 0;
    m_missCount = 0;
    m_lastEmitted.clear();
    m_lastSeen = 0;
    m_lastEmitTime = m_clock.elapsed() - kRepeatMs;
    m_flashUntil = 0;
}

void CameraScannerWidget::showPlaceholder(const QString& text)
{
    m_placeholderText->setText(text);
    m_stack->setCurrentIndex(0);
}

void CameraScannerWidget::hideEvent(QHideEvent* event)
{
    setActive(false);
    QWidget::hideEvent(event);
}

bool CameraScannerWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view && event->type() == QEvent::Resize)
        showFrame();
    return QWidget::eventFilter(watched, event);
}

// Shows the newest picture and handles a finished scan. The camera and the
// barcode search run in CameraReader's own threads, so this stays fast.
void CameraScannerWidget::onTick()
{
    if (!m_reader)
        return;

    const CameraReader::Snapshot snap = m_reader->poll();
    if (snap.failed) {
        const QString message = QString::fromStdString(snap.error);
        setActive(false);
        showPlaceholder(message);                    // keep the reason visible
        emit cameraFailed(message);
        return;
    }
    if (snap.frame.empty())                          // camera still starting
        return;

    const bool newScan = snap.scanId != m_lastScanId;
    if (!snap.newFrame && !newScan)                  // nothing new to show
        return;

    const cv::Mat& frame = snap.frame;               // 8-bit BGR
    QPixmap pixmap = QPixmap::fromImage(QImage(frame.data, frame.cols, frame.rows,
                                               int(frame.step), QImage::Format_BGR888));   // copies the pixels

    if (newScan) {
        m_lastScanId = snap.scanId;
        handleScan(snap.scan);
        if (!m_reader)                               // a slot may have stopped the camera
            return;
    }

    drawBoxes(pixmap);
    m_frame = pixmap;
    m_stack->setCurrentIndex(1);
    showFrame();
}

void CameraScannerWidget::handleScan(const CameraScanResult& scan)
{
    const bool hasValue = scan.value.has_value() && !scan.value->empty();
    const cv::Rect& shown = (hasValue && scan.readBox.area() > 0) ? scan.readBox : scan.box;
    m_box = toQRect(shown);

    if (!hasValue) {
        if (++m_missCount > kMissesBeforeForget) {   // lost it: forget the candidate
            m_pending.clear();
            m_pendingHits = 0;
        }
        return;
    }

    m_missCount = 0;
    const QString digits = QString::fromStdString(*scan.value);
    if (digits == m_pending) {
        ++m_pendingHits;
    } else {
        m_pending = digits;
        m_pendingHits = 1;
    }

    if (m_pendingHits >= 2)                          // same number twice = accepted
        accept(digits, toQRect(scan.readBox));
}

void CameraScannerWidget::accept(const QString& digits, const QRect& readBox)
{
    const qint64 now = m_clock.elapsed();
    m_readBox = readBox;
    m_flashUntil = now + kFlashMs;

    const bool sameAsBefore = digits == m_lastEmitted;
    const bool stillInPicture = now - m_lastSeen <= kSameCodeGapMs;
    const bool tooSoon = now - m_lastEmitTime < kRepeatMs;
    m_lastSeen = now;
    if (sameAsBefore && stillInPicture && tooSoon)
        return;

    m_lastEmitted = digits;
    m_lastEmitTime = now;
    playScanBeep();                       // classic scanner beep for every accepted scan
    emit barcodeScanned(digits);
}

// Accent frame while aiming; success colour around the area a number was read from.
void CameraScannerWidget::drawBoxes(QPixmap& frame) const
{
    const Theme::Colors& c = Theme::colors();
    if (m_clock.elapsed() < m_flashUntil && !m_readBox.isEmpty())
        drawBox(frame, m_readBox, c.success);
    else
        drawBox(frame, m_box, c.accent);
}

void CameraScannerWidget::showFrame()
{
    if (m_frame.isNull())
        return;

    const qreal dpr = devicePixelRatioF();
    QPixmap scaled = m_frame.scaled(m_view->size() * dpr, Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    m_view->setPixmap(scaled);
}
