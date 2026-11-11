#include "ReadPage.h"

#include "GeneratePage.h"
#include "Widgets.h"
#include "barcode_reader.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

namespace {

bool isImageSuffix(const QString& suffix)
{
    static const QStringList kSuffixes = { "png", "jpg", "jpeg", "bmp", "tif", "tiff" };
    return kSuffixes.contains(suffix.toLower());
}

// Small camera icon, painted so no image file is needed. Neutral grey so it
// is visible on both the light and the dark theme.
QIcon makeCameraIcon(int size)
{
    const qreal dpr = 2.0;
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor c(0x8a, 0x94, 0xa6);
    p.setPen(QPen(c, 1.6));
    p.setBrush(Qt::NoBrush);

    const qreal s = size;
    p.drawRoundedRect(QRectF(s * 0.08, s * 0.28, s * 0.84, s * 0.58), 3, 3);  // body
    p.setBrush(c);
    p.drawRoundedRect(QRectF(s * 0.30, s * 0.16, s * 0.26, s * 0.14), 2, 2);  // top bump
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(s * 0.5, s * 0.57), s * 0.19, s * 0.19);            // lens
    p.end();
    return QIcon(pm);
}

} // namespace

ReadPage::ReadPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("page");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 10, 40, 28);
    root->setSpacing(18);

    auto* header = new PageHeader(
        "Read barcode",
        "Drop a barcode image into the box, or click it to browse.",
        this);
    connect(header, &PageHeader::backClicked, this, &ReadPage::backRequested);
    root->addWidget(header);

    auto* body = new QHBoxLayout;
    body->setSpacing(20);

    // -------------------------------------------------------- left: drop zone
    m_drop = new DropZone(this);
    m_drop->setMinimumHeight(340);
    auto* dropLayout = new QVBoxLayout(m_drop);
    dropLayout->setContentsMargins(24, 24, 24, 20);
    dropLayout->setSpacing(8);

    // Shown while no image is loaded.
    m_placeholder = new QWidget(m_drop);
    auto* phLayout = new QVBoxLayout(m_placeholder);
    phLayout->setContentsMargins(0, 0, 0, 0);
    phLayout->setSpacing(8);

    auto* phIcon = new IconWidget(IconWidget::Scan, 76, m_placeholder);
    auto* phTitle = new QLabel("Drop a barcode image here", m_placeholder);
    phTitle->setObjectName("cardTitle");
    phTitle->setAlignment(Qt::AlignCenter);
    auto* phText = new QLabel("PNG, JPG or BMP. You can also click anywhere in this box.",
                              m_placeholder);
    phText->setObjectName("cardText");
    phText->setAlignment(Qt::AlignCenter);
    phText->setWordWrap(true);

    auto* browseBtn = new QPushButton(QString("Choose image") + kEllipsis, m_placeholder);
    browseBtn->setObjectName("primary");
    browseBtn->setCursor(Qt::PointingHandCursor);

    m_cameraBtn = new QPushButton(QString("Use camera"), m_placeholder);
    m_cameraBtn->setIcon(makeCameraIcon(18));
    m_cameraBtn->setIconSize(QSize(18, 18));
    m_cameraBtn->setCursor(Qt::PointingHandCursor);
    m_cameraBtn->setToolTip("Scan a barcode with the webcam");

    auto* phButtons = new QHBoxLayout;
    phButtons->setSpacing(10);
    phButtons->addStretch(1);
    phButtons->addWidget(browseBtn);
    phButtons->addWidget(m_cameraBtn);
    phButtons->addStretch(1);

    phLayout->addStretch(1);
    phLayout->addWidget(phIcon, 0, Qt::AlignHCenter);
    phLayout->addSpacing(6);
    phLayout->addWidget(phTitle);
    phLayout->addWidget(phText);
    phLayout->addSpacing(10);
    phLayout->addLayout(phButtons);
    phLayout->addStretch(1);

    // Shown once an image is loaded.
    m_image = new QLabel(m_drop);
    m_image->setAlignment(Qt::AlignCenter);
    m_image->setMinimumSize(260, 160);
    m_image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_image->installEventFilter(this);
    m_image->hide();

    m_caption = new QLabel(m_drop);
    m_caption->setObjectName("muted");
    m_caption->setAlignment(Qt::AlignCenter);
    m_caption->hide();

    dropLayout->addWidget(m_placeholder, 1);
    dropLayout->addWidget(m_image, 1);
    dropLayout->addWidget(m_caption);

    // ------------------------------------------------------- right: results
    QFrame* resultCard = makeCard(this);
    auto* resultLayout = new QVBoxLayout(resultCard);
    resultLayout->setContentsMargins(28, 26, 28, 24);
    resultLayout->setSpacing(10);

    auto* resultLabel = new QLabel("Decoded number", resultCard);
    resultLabel->setObjectName("fieldLabel");

    m_resultValue = new QLabel(resultCard);
    m_resultValue->setObjectName("resultValue");
    m_resultValue->setAlignment(Qt::AlignCenter);
    m_resultValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_fileName = new QLabel(resultCard);
    m_fileName->setObjectName("muted");
    m_fileName->setAlignment(Qt::AlignCenter);

    m_banner = new Banner(resultCard);

    m_copyBtn = new QPushButton("Copy number", resultCard);
    m_copyBtn->setObjectName("primary");
    m_copyBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn = new QPushButton("Clear", resultCard);
    m_clearBtn->setCursor(Qt::PointingHandCursor);

    auto* resultButtons = new QHBoxLayout;
    resultButtons->setSpacing(10);
    resultButtons->addWidget(m_copyBtn, 1);
    resultButtons->addWidget(m_clearBtn);

    auto* note = new QLabel(
        "Format: Code 128, Set C. Six digits. ",
        resultCard);
    note->setObjectName("muted");
    note->setWordWrap(true);

    resultLayout->addWidget(resultLabel);
    resultLayout->addStretch(1);
    resultLayout->addWidget(m_resultValue);
    resultLayout->addWidget(m_fileName);
    resultLayout->addSpacing(6);
    resultLayout->addWidget(m_banner);
    resultLayout->addSpacing(10);
    resultLayout->addLayout(resultButtons);
    resultLayout->addStretch(1);
    resultLayout->addWidget(note);

    body->addWidget(m_drop, 1);
    body->addWidget(resultCard, 1);
    root->addLayout(body, 1);

    // ------------------------------------------------------------- behaviour
    connect(m_drop, &DropZone::clicked, this, &ReadPage::browse);
    connect(m_drop, &DropZone::fileDropped, this, &ReadPage::loadFile);
    connect(browseBtn, &QPushButton::clicked, this, &ReadPage::browse);
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() { copyValue(m_lastValue); });
    connect(m_clearBtn, &QPushButton::clicked, this, &ReadPage::clearAll);

    m_cameraTimer = new QTimer(this);
    m_cameraTimer->setInterval(33);                    // ~30 frames per second
    connect(m_cameraTimer, &QTimer::timeout, this, &ReadPage::onCameraTick);
    connect(m_cameraBtn, &QPushButton::clicked, this, &ReadPage::startCamera);

    clearAll();
}

ReadPage::~ReadPage() = default;   // here, so cv::VideoCapture is a complete type

bool ReadPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_image && event->type() == QEvent::Resize)
        showImage();
    return QWidget::eventFilter(watched, event);
}

void ReadPage::browse()
{
    if (m_cap) {                    // clicking the picture while scanning = cancel
        stopCamera();
        m_caption->setText("Camera stopped   " + QString(kMiddleDot)
                           + "   click or drop to choose another");
        m_banner->showMessage("Camera stopped.", Banner::Info);
        return;
    }

    QString start = GeneratePage::outputDir();     // where our own barcodes are saved
    if (!QDir(start).exists())
        start = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    const QString path = QFileDialog::getOpenFileName(
        this, "Choose a barcode image", start,
        "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All files (*)");
    if (!path.isEmpty())
        loadFile(path);
}

void ReadPage::loadFile(const QString& path)
{
    stopCamera();
    const QFileInfo info(path);
    if (!info.isFile() || !isImageSuffix(info.suffix())) {
        m_banner->showMessage("Please choose an image file (PNG, JPG or BMP).", Banner::Error);
        return;
    }

    const QPixmap pixmap(path);
    if (pixmap.isNull()) {
        m_banner->showMessage("That image could not be opened.", Banner::Error);
        return;
    }

    m_pixmap = pixmap;
    m_caption->setText(info.fileName() + "   " + kMiddleDot + "   click or drop to choose another");
    showImage();

    decode(path);
}

void ReadPage::decode(const QString& path)
{
    const QString fileName = QFileInfo(path).fileName();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const std::optional<std::string> result = readBarcodeFile(toNativePath(path));
    QApplication::restoreOverrideCursor();

    m_fileName->setText(fileName);

    if (result.has_value() && !result->empty()) {
        const QString value = QString::fromStdString(*result);
        showResult(value);
        m_banner->showMessage("Barcode read successfully.", Banner::Success);
    } else {
        clearResult();
        m_banner->showMessage("No valid Code 128 barcode was found in this image. Try a "
                              "sharper picture where the whole barcode is visible.",
                              Banner::Error);
    }
}

void ReadPage::showImage()
{
    const bool has = !m_pixmap.isNull();
    m_placeholder->setVisible(!has);
    m_image->setVisible(has);
    m_caption->setVisible(has);
    if (has)
        m_image->setPixmap(fitPixmap(m_pixmap, m_image->size()));
}

void ReadPage::showResult(const QString& value)
{
    m_lastValue = value;
    m_resultValue->setText(value);
    m_resultValue->setProperty("empty", false);
    repolish(m_resultValue);
    m_copyBtn->setEnabled(true);
}

void ReadPage::clearResult()
{
    m_lastValue.clear();
    m_resultValue->setText("- - - - - -");
    m_resultValue->setProperty("empty", true);
    repolish(m_resultValue);
    m_copyBtn->setEnabled(false);
}

void ReadPage::copyValue(const QString& value)
{
    if (value.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(value);
    m_banner->showMessage("Copied " + value + " to the clipboard.", Banner::Info);
}

void ReadPage::clearAll()
{
    stopCamera();
    m_pixmap = QPixmap();
    m_image->clear();
    m_fileName->clear();
    m_banner->clearMessage();
    clearResult();
    showImage();
}

void ReadPage::clearPage()
{
    clearAll();
    m_caption->clear();
}

void ReadPage::hideEvent(QHideEvent* event)
{
    stopCamera();                   // never keep the webcam on in the background
    QWidget::hideEvent(event);
}

// ------------------------------------------------------------------- webcam

void ReadPage::startCamera()
{
    if (m_cap)
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);   // opening can take a moment
    auto cap = std::make_unique<cv::VideoCapture>();
#ifdef _WIN32
    bool ok = cap->open(0, cv::CAP_DSHOW);             // opens much faster than the default
    if (!ok)
        ok = cap->open(0);
#else
    const bool ok = cap->open(0);
#endif
    QApplication::restoreOverrideCursor();

    if (!ok || !cap->isOpened()) {
        m_banner->showMessage("No camera was found, or it is being used by another program.",
                              Banner::Error, 4000);
        return;
    }

    cap->set(cv::CAP_PROP_FRAME_WIDTH, 1280);          // more pixels = more reliable bars
    cap->set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    clearAll();                                        // also resets any old result
    m_cap = std::move(cap);
    m_frameCount = 0;
    m_caption->setText("Hold the barcode flat in front of the camera   " + QString(kMiddleDot)
                       + "   click to cancel");
    m_cameraTimer->start();
}

void ReadPage::stopCamera()
{
    if (m_cameraTimer)
        m_cameraTimer->stop();
    m_cap.reset();
}

void ReadPage::onCameraTick()
{
    if (!m_cap)
        return;

    cv::Mat frame;
    if (!m_cap->read(frame) || frame.empty()) {
        stopCamera();
        m_banner->showMessage("The camera stopped sending pictures.", Banner::Error, 4000);
        return;
    }
    if (frame.type() != CV_8UC3) {                     // QImage below expects BGR, 8 bit
        cv::Mat bgr;
        if (frame.channels() == 4)
            cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
        else if (frame.channels() == 1)
            cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
        else
            return;
        frame = bgr;
    }

    // live preview
    const QImage img(frame.data, frame.cols, frame.rows, int(frame.step),
                     QImage::Format_BGR888);
    m_pixmap = QPixmap::fromImage(img);                // copies the pixels
    showImage();

    // try to read every 3rd frame (keeps the preview smooth)
    if (++m_frameCount % 3 != 0)
        return;

    const std::optional<std::string> result = readBarcodeFrame(frame);
    if (result.has_value() && !result->empty()) {
        stopCamera();
        m_caption->setText("Captured from camera   " + QString(kMiddleDot)
                           + "   click or drop to choose another");
        m_fileName->setText("From camera");
        showResult(QString::fromStdString(*result));
        m_banner->showMessage("Barcode read successfully.", Banner::Success);
    }
}
