#include "ProductDialog.h"

#include "Neu.h"
#include "ProductDetailsView.h"
#include "Widgets.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QVBoxLayout>

ProductDialog::ProductDialog(const pos::Product& product, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Product");
    setMinimumWidth(380);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);       // the card already holds its shadow room
    layout->setSpacing(2);

    NeuFrame* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    auto* details = new ProductDetailsView(card);
    details->showProduct(product);
    cardLayout->addWidget(details);

    auto* closeBtn = new NeuButton("Close", NeuButton::Variant::Primary, this);
    closeBtn->setDefault(true);
    connect(closeBtn, &NeuButton::clicked, this, &QDialog::accept);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(closeBtn);

    layout->addWidget(card);
    layout->addLayout(buttons);
}

void ProductDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        accept();
        return;
    default:
        QDialog::keyPressEvent(event);
    }
}
