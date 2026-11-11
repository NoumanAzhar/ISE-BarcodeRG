
#include "MenuPage.h"

#include "Widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFont>

#include <opencv2/core/version.hpp>

MenuPage::MenuPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("page");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 10, 40, 24);
    root->setSpacing(0);

    root->addStretch(2);

    // ---- hero ----
    auto* logo = new IconWidget(IconWidget::Logo, 84, this);
    root->addWidget(logo, 0, Qt::AlignHCenter);
    root->addSpacing(16);

    auto* title = new QLabel("BarcodeRG", this);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);
    root->addSpacing(6);

    auto* subtitle = new QLabel("Generate and read Code 128 barcodes.\nChoose what you want to do.", this);
    subtitle->setObjectName("subtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    root->addWidget(subtitle);
    root->addSpacing(34);

    // ---- the two main actions ----
    auto* generateCard = new ActionCard(
        IconWidget::Generate, "Generate barcode",
        "Type a 6-digit number and get a barcode you can see, save and print.", this);
    auto* readCard = new ActionCard(
        IconWidget::Scan, "Read barcode",
        "Open or drop a barcode image and get the number back.", this);

    for (ActionCard* card : { generateCard, readCard }) {
        card->setMinimumSize(300, 220);
        card->setMaximumWidth(400);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    auto* cards = new QHBoxLayout;
    cards->setSpacing(22);
    cards->addStretch(1);
    cards->addWidget(generateCard, 3);
    cards->addWidget(readCard, 3);
    cards->addStretch(1);
    root->addLayout(cards);
    root->addSpacing(26);

    // ---- Exit action ----

    // ---- Prominent Exit action ----
    auto* exitBtn = new QPushButton("EXIT", this);
    exitBtn->setObjectName("ghost");
    exitBtn->setCursor(Qt::PointingHandCursor);

    exitBtn->setFixedSize(220, 80);
    exitBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QFont exitFont = exitBtn->font();
    exitFont.setBold(true);
    exitFont.setPointSize(16);
    exitBtn->setFont(exitFont);

    exitBtn->setStyleSheet(
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 12px 28px;"
        "border-width: 2px;"
    );

    auto* links = new QHBoxLayout;
    links->addStretch(1);
    links->addWidget(exitBtn);
    links->addStretch(1);
    root->addLayout(links);

    root->addStretch(2);

    auto* footer = new QLabel(
        QString("Increment 1  %1  Code 128  %1  Qt %2  %1  OpenCV %3")
        .arg(kMiddleDot)
        .arg(QString::fromLatin1(qVersion()))
        .arg(QString::fromLatin1(CV_VERSION)),
        this);
    footer->setObjectName("footer");
    footer->setAlignment(Qt::AlignCenter);
    root->addWidget(footer);

    connect(generateCard, &ActionCard::clicked, this, &MenuPage::generateRequested);
    connect(readCard, &ActionCard::clicked, this, &MenuPage::readRequested);
    connect(exitBtn, &QPushButton::clicked, this, &MenuPage::exitRequested);
}
