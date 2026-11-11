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
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringList>
#include <QVBoxLayout>

namespace {

bool isImageSuffix(const QString& suffix)
{
    static const QStringList kSuffixes = { "png", "jpg", "jpeg", "bmp", "tif", "tiff" };
    return kSuffixes.contains(suffix.toLower());
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

    phLayout->addStretch(1);
    phLayout->addWidget(phIcon, 0, Qt::AlignHCenter);
    phLayout->addSpacing(6);
    phLayout->addWidget(phTitle);
    phLayout->addWidget(phText);
    phLayout->addSpacing(10);
    phLayout->addWidget(browseBtn, 0, Qt::AlignHCenter);
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

    clearAll();
}

bool ReadPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_image && event->type() == QEvent::Resize)
        showImage();
    return QWidget::eventFilter(watched, event);
}

void ReadPage::browse()
{
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
