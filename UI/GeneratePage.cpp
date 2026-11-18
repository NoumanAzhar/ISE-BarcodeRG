#include "GeneratePage.h"

#include "Neu.h"
#include "Widgets.h"
#include "barcode_render.h"
#include "code128.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QUrl>
#include <QVBoxLayout>

namespace {

// Size of one module (the thinnest bar) in pixels, and bar height, used for the
// image that is saved to disk. Bigger bars are easier for the reader.
const int kModuleWidth = 4;
const int kBarHeight = 140;

const char* const kEmptyPreviewText = "Your barcode will appear here";

QImage matToQImage(const cv::Mat& m)
{
    if (m.empty())
        return QImage();

    if (m.type() == CV_8UC1) {
        return QImage(m.data, m.cols, m.rows, static_cast<int>(m.step),
                      QImage::Format_Grayscale8).copy();
    }
    if (m.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(m, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy();
    }
    if (m.type() == CV_8UC4) {
        cv::Mat rgba;
        cv::cvtColor(m, rgba, cv::COLOR_BGRA2RGBA);
        return QImage(rgba.data, rgba.cols, rgba.rows, static_cast<int>(rgba.step),
                      QImage::Format_RGBA8888).copy();
    }
    return QImage();
}

// "483920" -> "4 8 3 9 2 0"
QString spaced(const QString& digits)
{
    QString out;
    for (int i = 0; i < digits.size(); ++i) {
        if (i > 0)
            out += ' ';
        out += digits[i];
    }
    return out;
}

} // namespace

QString GeneratePage::outputDir()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("barcodes");
}

GeneratePage::GeneratePage(QWidget* parent)
    : PageBase(parent)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room
    root->addWidget(buildInputCard(), 4);
    root->addWidget(buildPreviewCard(), 6);

    onTextChanged(QString());
}

// ---------------------------------------------------------------- building

QWidget* GeneratePage::buildInputCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    auto* label = makeLabel("Number", "fieldLabel", card);

    m_input = new NeuLineEdit(card);
    m_input->setLarge(true);
    m_input->setMaxLength(6);
    m_input->setAlignment(Qt::AlignCenter);
    m_input->setPlaceholderText("e.g. 483920");
    m_input->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,6}"), m_input));

    m_hint = makeLabel(QString(), "hint", card);
    m_hint->setWordWrap(true);
    m_counter = makeLabel("0 / 6", "muted", card);

    auto* hintRow = new QHBoxLayout;
    hintRow->addWidget(m_hint, 1);
    hintRow->addWidget(m_counter, 0, Qt::AlignTop);

    m_generateBtn = new NeuButton("Generate", NeuButton::Variant::Primary, card);
    auto* randomBtn = new NeuButton("Random", NeuButton::Variant::Normal, card);
    randomBtn->setToolTip("Fill in a random valid number and generate it");

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    buttons->addWidget(m_generateBtn, 1);
    buttons->addWidget(randomBtn);

    layout->addWidget(label);
    layout->addWidget(m_input);
    layout->addLayout(hintRow);
    layout->addLayout(buttons);
    layout->addStretch(1);

    connect(m_input, &QLineEdit::textChanged, this, &GeneratePage::onTextChanged);
    connect(m_input, &QLineEdit::returnPressed, this, &GeneratePage::generate);
    connect(m_generateBtn, &NeuButton::clicked, this, &GeneratePage::generate);
    connect(randomBtn, &NeuButton::clicked, this, &GeneratePage::randomNumber);
    return card;
}

QWidget* GeneratePage::buildPreviewCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    // White paper so the barcode always looks like a printed one.
    auto* paper = new QFrame(card);
    paper->setObjectName("paper");
    paper->setStyleSheet("QFrame#paper { background: #ffffff; border-radius: 14px; }");
    auto* paperLayout = new QVBoxLayout(paper);
    paperLayout->setContentsMargins(18, 18, 18, 14);
    paperLayout->setSpacing(6);

    m_preview = new QLabel(kEmptyPreviewText, paper);
    m_preview->setObjectName("paperText");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(320, 150);
    m_preview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_preview->installEventFilter(this);

    m_digitsLabel = makeLabel(QString(), "paperDigits", paper);
    m_digitsLabel->setAlignment(Qt::AlignCenter);

    paperLayout->addWidget(m_preview, 1);
    paperLayout->addWidget(m_digitsLabel);

    m_path = makeLabel(QString(), "muted", card);
    m_path->setWordWrap(true);
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_banner = new Banner(card);

    m_saveBtn = new NeuButton(QString("Save as") + kEllipsis, NeuButton::Variant::Normal, card);
    m_copyBtn = new NeuButton("Copy image", NeuButton::Variant::Normal, card);
    m_folderBtn = new NeuButton("Open folder", NeuButton::Variant::Normal, card);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(2);
    actions->addWidget(m_saveBtn);
    actions->addWidget(m_copyBtn);
    actions->addWidget(m_folderBtn);
    actions->addStretch(1);

    layout->addWidget(paper, 1);
    layout->addWidget(m_path);
    layout->addWidget(m_banner);
    layout->addLayout(actions);

    connect(m_saveBtn, &NeuButton::clicked, this, &GeneratePage::saveAs);
    connect(m_copyBtn, &NeuButton::clicked, this, &GeneratePage::copyImage);
    connect(m_folderBtn, &NeuButton::clicked, this, &GeneratePage::openFolder);

    setActionsEnabled(false);
    return card;
}

// ------------------------------------------------------------- page hooks

void GeneratePage::onActivated()
{
    clearPreview();
    m_input->setFocus();
}

bool GeneratePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_preview && event->type() == QEvent::Resize)
        updatePreview();
    return PageBase::eventFilter(watched, event);
}

// ------------------------------------------------------------------- input

void GeneratePage::onTextChanged(const QString& text)
{
    const int n = static_cast<int>(text.size());
    m_counter->setText(QString("%1 / 6").arg(n));

    const bool valid = validate6Digits(text.toStdString());

    QString state;       // "", "valid" or "invalid" (drives the colours)
    QString hint;
    if (n == 0) {
        hint = "Type 6 digits. The first digit cannot be 0.";
    } else if (text.at(0) == QLatin1Char('0')) {
        state = "invalid";
        hint = "The first digit must be 1-9.";
    } else if (n < 6) {
        hint = (6 - n == 1) ? QString("1 more digit to go.")
                            : QString("%1 more digits to go.").arg(6 - n);
    } else if (valid) {
        state = "valid";
        hint = "Looks good. Click Generate.";
    } else {
        state = "invalid";
        hint = "Enter exactly 6 digits.";
    }

    m_input->setState(state);
    m_hint->setProperty("state", state);
    repolish(m_hint);

    m_hint->setText(hint);
    m_generateBtn->setEnabled(valid);
}

void GeneratePage::randomNumber()
{
    const int value = QRandomGenerator::global()->bounded(100000, 1000000);   // 100000..999999
    m_input->setText(QString::number(value));
    generate();
}

// ---------------------------------------------------------------- generate

void GeneratePage::generate()
{
    const QString digits = m_input->text();
    const std::string text = digits.toStdString();
    if (!validate6Digits(text))
        return;

    const std::string modules = encodeCode128C(text);
    if (modules.empty()) {
        m_banner->showMessage("The encoder could not encode this number.", Banner::Error);
        return;
    }

    m_image = matToQImage(renderBarcode(modules, kModuleWidth, kBarHeight));
    if (m_image.isNull()) {
        m_banner->showMessage("The barcode image could not be drawn.", Banner::Error);
        return;
    }

    m_currentDigits = digits;
    m_digitsLabel->setText(spaced(digits));
    updatePreview();
    setActionsEnabled(true);

    QDir().mkpath(outputDir());
    const QString path = QDir(outputDir()).filePath(digits + ".png");
    if (m_image.save(path)) {
        m_path->setText(QDir::toNativeSeparators(path));
        m_banner->showMessage(QString(kCheck) + "  Barcode generated and saved.", Banner::Success);
    } else {
        m_path->clear();
        m_banner->showMessage("Barcode generated, but it could not be saved to "
                              + QDir::toNativeSeparators(outputDir()), Banner::Warning);
    }
}

void GeneratePage::updatePreview()
{
    if (m_image.isNull())
        return;
    m_preview->setPixmap(fitPixmap(QPixmap::fromImage(m_image), m_preview->size()));
}

void GeneratePage::clearPreview()
{
    m_input->clear();

    m_image = QImage();
    m_currentDigits.clear();

    m_preview->clear();
    m_preview->setText(kEmptyPreviewText);
    m_digitsLabel->clear();
    m_path->clear();
    m_banner->clearMessage();
    setActionsEnabled(false);
}

void GeneratePage::setActionsEnabled(bool enabled)
{
    m_saveBtn->setEnabled(enabled);
    m_copyBtn->setEnabled(enabled);
    m_folderBtn->setEnabled(enabled);
}

// ----------------------------------------------------------------- actions

void GeneratePage::saveAs()
{
    if (m_image.isNull())
        return;

    const QString suggested = QDir(outputDir()).filePath(m_currentDigits + ".png");
    QString path = QFileDialog::getSaveFileName(
        this, "Save barcode", suggested,
        "PNG image (*.png);;JPEG image (*.jpg *.jpeg);;Bitmap (*.bmp)");
    if (path.isEmpty())
        return;

    if (QFileInfo(path).suffix().isEmpty())
        path += ".png";

    if (m_image.save(path))
        m_banner->showMessage("Saved a copy to " + QDir::toNativeSeparators(path), Banner::Success);
    else
        m_banner->showMessage("Could not save to " + QDir::toNativeSeparators(path), Banner::Error);
}

void GeneratePage::copyImage()
{
    if (m_image.isNull())
        return;
    QGuiApplication::clipboard()->setImage(m_image);
    m_banner->showMessage("Barcode image copied to the clipboard.", Banner::Info);
}

void GeneratePage::openFolder()
{
    QDir().mkpath(outputDir());
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir()));
}
