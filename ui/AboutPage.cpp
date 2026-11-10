#include "AboutPage.h"

#include "Widgets.h"

#include <QLabel>
#include <QVBoxLayout>

#include <opencv2/core/version.hpp>

AboutPage::AboutPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("page");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 10, 40, 28);
    root->setSpacing(18);

    auto* header = new PageHeader("About", "What this app does and how it works.", this);
    connect(header, &PageHeader::backClicked, this, &AboutPage::backRequested);
    root->addWidget(header);

    QFrame* card = makeCard(this);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(30, 26, 30, 26);

    const QString html = QString(
        "<p><b>BarcodeRG</b> is the barcode part of our ISE term project: a desktop tool "
        "for a small store that turns product numbers into barcodes and reads them back. "
        "This is Increment 1.</p>"
        "<p><b>Generate</b> &ndash; type a 6-digit number (first digit not 0). The app draws a "
        "Code 128 barcode, shows it and saves it as a PNG image.</p>"
        "<p><b>Read</b> &ndash; open or drop an image of a barcode made by this app. The number "
        "is decoded and shown on screen.</p>"
        "<p><b>How the barcode is built</b><br>"
        "Code 128 Set C stores two digits per symbol, so six digits become three symbols. "
        "The barcode is: Start C, three data symbols, a checksum symbol and a Stop symbol. "
        "The encoder and the reader are our own code &ndash; no barcode library is used.</p>"
        "<p><b>Built with</b> C++17, Qt %1 (interface) and OpenCV %2 (image handling).</p>")
        .arg(QString::fromLatin1(qVersion()))
        .arg(QString::fromLatin1(CV_VERSION));

    auto* text = new QLabel(html, card);
    text->setObjectName("aboutText");
    text->setTextFormat(Qt::RichText);
    text->setWordWrap(true);
    text->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    cardLayout->addWidget(text);

    root->addWidget(card);
    root->addStretch(1);
}
