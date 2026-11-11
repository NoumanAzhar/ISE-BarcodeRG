#include "GeneratePage.h"

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
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {

// Size of one module (the thinnest bar) in pixels, and bar height, used for the
// image that is saved to disk. Bigger bars are easier for the reader.
const int kModuleWidth = 4;
const int kBarHeight = 140;

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
    : QWidget(parent)
{
    setObjectName("page");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 10, 40, 28);
    root->setSpacing(18);

    auto* header = new PageHeader(
        "Generate barcode",
        "Type a 6-digit number (the first digit cannot be 0). The barcode is "
        "generated instantly and shown as a preview. You can save it manually.",
        this);
    connect(header, &PageHeader::backClicked, this, &GeneratePage::backRequested);
    root->addWidget(header);

    auto* body = new QHBoxLayout;
    body->setSpacing(20);

    // ------------------------------------------------------------------ left
    QFrame* left = makeCard(this);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(26, 24, 26, 24);
    leftLayout->setSpacing(10);

    auto* fieldLabel = new QLabel("Number to encode", left);
    fieldLabel->setObjectName("fieldLabel");

    m_input = new QLineEdit(left);
    m_input->setPlaceholderText("e.g. 483920");
    m_input->setMaxLength(6);
    m_input->setAlignment(Qt::AlignCenter);
    m_input->setClearButtonEnabled(true);
    m_input->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,6}"), m_input));

    m_hint = new QLabel(left);
    m_hint->setObjectName("hint");
    m_hint->setWordWrap(true);

    m_counter = new QLabel("0 / 6", left);
    m_counter->setObjectName("muted");

    auto* hintRow = new QHBoxLayout;
    hintRow->addWidget(m_hint, 1);
    hintRow->addWidget(m_counter, 0, Qt::AlignTop);

    m_generateBtn = new QPushButton("Generate", left);
    m_generateBtn->setObjectName("primary");
    m_generateBtn->setCursor(Qt::PointingHandCursor);

    m_randomBtn = new QPushButton("Random number", left);
    m_randomBtn->setCursor(Qt::PointingHandCursor);
    m_randomBtn->setToolTip("Fill in a random valid number and generate it");

    auto* buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(10);
    buttonRow->addWidget(m_generateBtn, 1);
    buttonRow->addWidget(m_randomBtn);

    auto* note = new QLabel(
        "Format: Code 128, Set C. Six digits become three symbols, plus a start "
        "symbol, a checksum and a stop symbol.",
        left);
    note->setObjectName("muted");
    note->setWordWrap(true);

    leftLayout->addWidget(fieldLabel);
    leftLayout->addWidget(m_input);
    leftLayout->addLayout(hintRow);
    leftLayout->addSpacing(8);
    leftLayout->addLayout(buttonRow);
    leftLayout->addStretch(1);
    leftLayout->addWidget(note);

    // ----------------------------------------------------------------- right
    QFrame* right = makeCard(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(22, 22, 22, 22);
    rightLayout->setSpacing(12);

    // White "paper" so the barcode always looks like a printed one.
    auto* paper = new QFrame(right);
    paper->setObjectName("paper");
    auto* paperLayout = new QVBoxLayout(paper);
    paperLayout->setContentsMargins(18, 18, 18, 14);
    paperLayout->setSpacing(6);

    m_preview = new QLabel("Your barcode will appear here", paper);
    m_preview->setObjectName("paperText");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(320, 150);
    m_preview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_preview->installEventFilter(this);

    m_digitsLabel = new QLabel(paper);
    m_digitsLabel->setObjectName("paperDigits");
    m_digitsLabel->setAlignment(Qt::AlignCenter);

    paperLayout->addWidget(m_preview, 1);
    paperLayout->addWidget(m_digitsLabel);

    m_banner = new Banner(right);

    m_path = new QLabel(right);
    m_path->setObjectName("muted");
    m_path->setWordWrap(true);
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_saveBtn = new QPushButton(QString("Save as") + kEllipsis, right);
    m_copyBtn = new QPushButton("Copy image", right);
    m_folderBtn = new QPushButton("Open folder", right);
    for (QPushButton* b : { m_saveBtn, m_copyBtn, m_folderBtn }) {
        b->setCursor(Qt::PointingHandCursor);
        b->setEnabled(false);
    }

    auto* actionRow = new QHBoxLayout;
    actionRow->setSpacing(10);
    actionRow->addWidget(m_saveBtn);
    actionRow->addWidget(m_copyBtn);
    actionRow->addWidget(m_folderBtn);
    actionRow->addStretch(1);

    rightLayout->addWidget(paper, 1);
    rightLayout->addWidget(m_banner);
    rightLayout->addWidget(m_path);
    rightLayout->addLayout(actionRow);

    body->addWidget(left, 4);
    body->addWidget(right, 6);
    root->addLayout(body, 1);

    // ------------------------------------------------------------- behaviour
    connect(m_input, &QLineEdit::textChanged, this, &GeneratePage::onTextChanged);
    connect(m_input, &QLineEdit::returnPressed, this, &GeneratePage::generate);
    connect(m_generateBtn, &QPushButton::clicked, this, &GeneratePage::generate);
    connect(m_randomBtn, &QPushButton::clicked, this, &GeneratePage::randomNumber);
    connect(m_saveBtn, &QPushButton::clicked, this, &GeneratePage::saveAs);
    connect(m_copyBtn, &QPushButton::clicked, this, &GeneratePage::copyImage);
    connect(m_folderBtn, &QPushButton::clicked, this, &GeneratePage::openFolder);

    onTextChanged(QString());
}

void GeneratePage::focusInput()
{
    m_input->setFocus();
    m_input->selectAll();
}

void GeneratePage::clearPreview()
{
    if (m_input) {
        m_input->clear();
    }

    m_image = QImage();
    m_currentDigits.clear();

    m_preview->clear();
    m_preview->setText("Your barcode will appear here");

    m_digitsLabel->clear();
    m_path->clear();
    m_banner->clearMessage();

    m_saveBtn->setEnabled(false);
    m_copyBtn->setEnabled(false);
    m_folderBtn->setEnabled(false);
}

bool GeneratePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_preview && event->type() == QEvent::Resize)
        updatePreview();
    return QWidget::eventFilter(watched, event);
}

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
        hint = "Looks good. Press Enter or click Generate.";
    } else {
        state = "invalid";
        hint = "Enter exactly 6 digits.";
    }

    m_input->setProperty("state", state);
    m_hint->setProperty("state", state);
    repolish(m_input);
    repolish(m_hint);

    m_hint->setText(hint);
    m_generateBtn->setEnabled(valid);
}

void GeneratePage::randomNumber()
{
    const int value = QRandomGenerator::global()->bounded(100000, 1000000);   // 100000..999999
    m_input->setText(QString::number(value));
}

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

    const cv::Mat img = renderBarcode(modules, kModuleWidth, kBarHeight);
    m_image = matToQImage(img);
    if (m_image.isNull()) {
        m_banner->showMessage("The barcode image could not be drawn.", Banner::Error);
        return;
    }

    m_currentDigits = digits;
    m_digitsLabel->setText(spaced(digits));
    updatePreview();

    m_saveBtn->setEnabled(true);
    m_copyBtn->setEnabled(true);
    m_folderBtn->setEnabled(true);

    // Show preview only. Saving is available through the Save as button.
    m_path->clear();

    // Clear the hint text after generating
    m_hint->clear();
    m_hint->setProperty("state", QString());
    repolish(m_hint);

    m_banner->showMessage(
        "Barcode generated successfully.",
        Banner::Success);
}

void GeneratePage::updatePreview()
{
    if (m_image.isNull())
        return;
    m_preview->setPixmap(fitPixmap(QPixmap::fromImage(m_image), m_preview->size()));
}

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
