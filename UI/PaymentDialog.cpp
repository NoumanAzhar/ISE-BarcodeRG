#include "PaymentDialog.h"

#include "Format.h"
#include "Neu.h"
#include "Widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>
#include <QVBoxLayout>

namespace {

enum ModeIndex { CashIndex, CardIndex, WalletIndex };

} // namespace

PaymentDialog::PaymentDialog(double totalPayable, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Payment");
    setMinimumWidth(400);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);       // the card already holds its shadow room
    layout->setSpacing(2);

    NeuFrame* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(8);

    m_mode = new NeuSegmented(QStringList{ "Cash", "Card", "Wallet" }, card);
    m_merchant = new NeuLineEdit(card);
    m_merchant->setPlaceholderText("Merchant, e.g. ABL or JazzCash");

    cardLayout->addWidget(makeLabel("Total", "sectionTitle", card));
    cardLayout->addWidget(makeLabel(formatPkr(totalPayable), "totalValue", card));
    cardLayout->addSpacing(6);
    cardLayout->addWidget(m_mode);
    cardLayout->addWidget(m_merchant);

    auto* cancel = new NeuButton("Cancel", NeuButton::Variant::Normal, this);
    cancel->setAutoDefault(false);
    m_confirm = new NeuButton("Confirm payment", NeuButton::Variant::Primary, this);
    m_confirm->setDefault(true);                  // Enter confirms when enabled

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    buttons->addStretch(1);
    buttons->addWidget(cancel);
    buttons->addWidget(m_confirm);

    layout->addWidget(card);
    layout->addLayout(buttons);

    connect(cancel, &NeuButton::clicked, this, &QDialog::reject);
    connect(m_confirm, &NeuButton::clicked, this, &QDialog::accept);
    connect(m_mode, &NeuSegmented::currentChanged, this, [this]() {
        updateState();
        if (needsMerchant())
            m_merchant->setFocus();
    });
    connect(m_merchant, &NeuLineEdit::textChanged, this, &PaymentDialog::updateState);

    m_mode->setCurrentIndex(CashIndex);
    updateState();
}

pos::PaymentMode PaymentDialog::mode() const
{
    switch (m_mode->currentIndex()) {
    case CardIndex:   return pos::PaymentMode::Card;
    case WalletIndex: return pos::PaymentMode::Wallet;
    default:          return pos::PaymentMode::Cash;
    }
}

QString PaymentDialog::merchant() const
{
    return needsMerchant() ? m_merchant->text().trimmed() : QString();
}

bool PaymentDialog::needsMerchant() const
{
    return m_mode->currentIndex() != CashIndex;
}

void PaymentDialog::updateState()
{
    m_merchant->setVisible(needsMerchant());
    m_confirm->setEnabled(!needsMerchant() || !m_merchant->text().trimmed().isEmpty());
}
