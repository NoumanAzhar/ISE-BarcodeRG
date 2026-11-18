#include "ReadPage.h"

#include "AppServices.h"
#include "CameraScannerWidget.h"
#include "Format.h"
#include "Neu.h"
#include "ProductDetailsView.h"
#include "Theme.h"
#include "Widgets.h"
#include "barcode_reader.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStringList>
#include <QVBoxLayout>

namespace {

constexpr int kErrorMs = 4000;

bool isImageSuffix(const QString& suffix)
{
    static const QStringList kSuffixes = { "png", "jpg", "jpeg", "bmp", "tif", "tiff" };
    return kSuffixes.contains(suffix.toLower());
}

} // namespace

ReadPage::ReadPage(pos::AppServices& services, ReadMode mode, QWidget* parent)
    : PageBase(parent), m_services(services), m_mode(mode)
{
    m_lastDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room
    root->addWidget(buildSourceCard(), 1);
    root->addWidget(buildResultCard(), 1);

    clearAll();
}

// ---------------------------------------------------------------- building

QWidget* ReadPage::buildSourceCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    m_camera = new CameraScannerWidget(card);
    connect(m_camera, &CameraScannerWidget::barcodeScanned, this, &ReadPage::onCameraScanned);
    connect(m_camera, &CameraScannerWidget::cameraFailed, this, &ReadPage::onCameraFailed);

    if (m_mode == ReadMode::CameraOnly) {
        layout->addWidget(m_camera, 1);
        return card;
    }

    m_source = new NeuSegmented(QStringList{ "Image", "Camera" }, card);
    m_stack = new QStackedWidget(card);
    m_stack->addWidget(buildDropZone());         // index 0
    m_stack->addWidget(m_camera);                // index 1

    layout->addWidget(m_source);
    layout->addWidget(m_stack, 1);

    connect(m_source, &NeuSegmented::currentChanged, this, &ReadPage::setSource);
    return card;
}

QWidget* ReadPage::buildDropZone()
{
    m_drop = new DropZone(this);
    m_drop->setMinimumHeight(300);

    auto* layout = new QVBoxLayout(m_drop);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(8);

    // Shown while no image is loaded.
    m_placeholder = new QWidget(m_drop);
    auto* ph = new QVBoxLayout(m_placeholder);
    ph->setContentsMargins(0, 0, 0, 0);
    ph->setSpacing(10);

    auto* icon = new IconWidget(IconWidget::Scan, 64);
    auto* text = makeLabel("Drop a barcode image or click to choose", "cardText", m_placeholder);
    text->setAlignment(Qt::AlignCenter);
    text->setWordWrap(true);

    ph->addStretch(1);
    ph->addWidget(icon, 0, Qt::AlignHCenter);
    ph->addWidget(text);
    ph->addStretch(1);

    // Shown once an image is loaded.
    m_image = new QLabel(m_drop);
    m_image->setAlignment(Qt::AlignCenter);
    m_image->setMinimumSize(200, 120);
    m_image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_image->installEventFilter(this);

    m_caption = makeLabel(QString(), "muted", m_drop);
    m_caption->setAlignment(Qt::AlignCenter);

    // Clicks on the content must reach the drop zone.
    for (QWidget* w : { m_placeholder, static_cast<QWidget*>(m_image), static_cast<QWidget*>(m_caption) })
        w->setAttribute(Qt::WA_TransparentForMouseEvents);

    layout->addWidget(m_placeholder, 1);
    layout->addWidget(m_image, 1);
    layout->addWidget(m_caption);

    connect(m_drop, &DropZone::clicked, this, &ReadPage::browse);
    connect(m_drop, &DropZone::fileDropped, this, &ReadPage::loadFile);
    return m_drop;
}

QWidget* ReadPage::buildResultCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(8);

    auto* title = makeLabel("BARCODE", "sectionTitle", card);

    m_value = makeLabel(QString(), "resultValue", card);
    m_value->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_details = new ProductDetailsView(card);

    m_banner = new Banner(card);

    m_copyBtn = new NeuButton("Copy", NeuButton::Variant::Primary, card);
    auto* clearBtn = new NeuButton("Clear", NeuButton::Variant::Normal, card);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    buttons->addWidget(m_copyBtn, 1);
    buttons->addWidget(clearBtn);

    layout->addWidget(title);
    layout->addWidget(m_value);
    layout->addWidget(m_details, 1);
    layout->addWidget(m_banner);
    layout->addLayout(buttons);

    connect(m_copyBtn, &NeuButton::clicked, this, &ReadPage::copyDigits);
    connect(clearBtn, &NeuButton::clicked, this, &ReadPage::clearAll);
    return card;
}

// ------------------------------------------------------------- page hooks

void ReadPage::onActivated()
{
    m_active = true;
    setSource(m_source ? m_source->currentIndex() : 1);
}

void ReadPage::onDeactivated()
{
    m_active = false;
    m_camera->setActive(false);
}

void ReadPage::resetState()
{
    clearAll();     // digits, product details, loaded image and messages
}

bool ReadPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_image && event->type() == QEvent::Resize)
        showImage();
    return PageBase::eventFilter(watched, event);
}

// ------------------------------------------------------------------ source

void ReadPage::setSource(int index)
{
    if (m_stack)
        m_stack->setCurrentIndex(index == 1 ? 1 : 0);
    m_camera->setActive(false);
}

void ReadPage::browse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Choose a barcode image", m_lastDir,
        "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All files (*)");
    if (path.isEmpty())
        return;
    m_lastDir = QFileInfo(path).absolutePath();
    loadFile(path);
}

void ReadPage::loadFile(const QString& path)
{
    const QFileInfo info(path);
    if (!info.isFile() || !isImageSuffix(info.suffix())) {
        showError("Please choose an image file (PNG, JPG or BMP).");
        return;
    }

    const QPixmap pixmap(path);
    if (pixmap.isNull()) {
        showError("That image could not be opened.");
        return;
    }

    m_pixmap = pixmap;
    m_caption->setText(info.fileName());
    showImage();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const std::optional<std::string> result = readBarcodeFile(toNativePath(path));
    QApplication::restoreOverrideCursor();

    if (result.has_value() && !result->empty()) {
        showDigits(qs(*result));
    } else {
        clearResult();
        showError("No valid Code 128 barcode found. Try a sharper picture "
                  "where the whole barcode is visible.");
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

void ReadPage::onCameraScanned(const QString& digits)
{
    if (digits != m_digits)                      // the camera repeats the same read
        showDigits(digits);
}

void ReadPage::onCameraFailed(const QString& message)
{
    showError(message);
    if (m_source) {                              // fall back to the image source
        setSource(0);
        m_source->setCurrentIndex(0);
    }
}

// ------------------------------------------------------------------ result

void ReadPage::showDigits(const QString& digits)
{
    m_banner->clearMessage();
    m_digits = digits;
    m_value->setText(digits);
    m_value->setProperty("empty", false);
    repolish(m_value);
    m_copyBtn->setEnabled(true);

    if (const auto product = m_services.catalog().lookup(ss(digits)))
        m_details->showProduct(*product);
    else
        m_details->showMessage("No product has this barcode");
}

void ReadPage::clearResult()
{
    m_digits.clear();
    m_value->setText("- - - - - -");
    m_value->setProperty("empty", true);
    repolish(m_value);
    m_copyBtn->setEnabled(false);
    m_details->clear();
}

void ReadPage::clearAll()
{
    clearResult();
    m_banner->clearMessage();
    if (m_image) {
        m_pixmap = QPixmap();
        showImage();
    }
}

void ReadPage::copyDigits()
{
    if (m_digits.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(m_digits);
    m_banner->showMessage(QString(kCheck) + "  Copied", Banner::Success);
}

void ReadPage::showError(const QString& text)
{
    m_banner->showMessage(text, Banner::Error, kErrorMs);
}
