#include "HomePage.h"

#include "Neu.h"
#include "Widgets.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

HomePage::HomePage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("page");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 10, 40, 28);
    root->setSpacing(0);
    root->addStretch(2);

    // ---- brand ----
    auto* logo = new IconWidget(IconWidget::Logo, 76, this);
    root->addWidget(logo, 0, Qt::AlignHCenter);
    root->addSpacing(14);

    auto* title = makeLabel("BarcodeRG", "title", this);
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);
    root->addSpacing(26);

    // ---- the two roles ----
    auto* cashierCard = new ActionCard(IconWidget::Cashier, "Cashier",
                                       "Scan, hold and bill orders", this);
    auto* managerCard = new ActionCard(IconWidget::Manager, "Manager",
                                       "Products, stock, discounts and reports", this);

    for (ActionCard* card : { cashierCard, managerCard }) {
        card->setMinimumSize(320, 250);
        card->setMaximumWidth(420);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    auto* cards = new QHBoxLayout;
    cards->setSpacing(18);
    cards->addStretch(1);
    cards->addWidget(cashierCard, 3);
    cards->addWidget(managerCard, 3);
    cards->addStretch(1);
    root->addLayout(cards);
    root->addSpacing(14);

    // ---- exit ----
    auto* exitBtn = new NeuButton("Exit", NeuButton::Variant::Normal, this);
    exitBtn->setMinimumWidth(180);
    auto* exitRow = new QHBoxLayout;
    exitRow->addStretch(1);
    exitRow->addWidget(exitBtn);
    exitRow->addStretch(1);
    root->addLayout(exitRow);

    root->addStretch(2);

    connect(cashierCard, &ActionCard::clicked, this, &HomePage::cashierRequested);
    connect(managerCard, &ActionCard::clicked, this, &HomePage::managerRequested);
    connect(exitBtn, &QPushButton::clicked, this, &HomePage::exitRequested);
}

void HomePage::clearFocusState()
{
    const QList<QWidget*> widgets = findChildren<QWidget*>();
    for (QWidget* widget : widgets)
        widget->clearFocus();
}
