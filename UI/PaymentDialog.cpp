#include "PaymentDialog.h"

#include "Format.h"
#include "Neu.h"
#include "Theme.h"
#include "Widgets.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

namespace {

enum ModeIndex { CashIndex, CardIndex, WalletIndex };

const QStringList kBanks = { "Meezan Bank", "HBL", "MCB", "UBL", "ABL" };
const QStringList kWallets = { "JazzCash", "EasyPaisa", "NayaPay" };

// Drop-down look that matches the Neu inputs in the current theme.
QString comboStyle()
{
    const Theme::Colors& c = Theme::colors();
    return QString(
        "QComboBox { background: %1; color: %2; border: 1px solid %3; border-radius: 12px;"
        "            padding: 0px 14px; min-height: 44px; font-size: 15px; font-weight: 500; }"
        "QComboBox:focus, QComboBox:on { border: 1px solid %4; }"
        "QComboBox::drop-down { border: none; width: 34px; }"
        "QComboBox QAbstractItemView { background: %1; color: %2; border: 1px solid %3;"
        "            selection-background-color: %4; selection-color: %5; outline: none; padding: 4px; }"
        "QComboBox QAbstractItemView::item { min-height: 32px; padding-left: 10px; }")
        .arg(c.surface2.name(), c.text.name(), c.border.name(), c.accent.name(), c.onAccent.name());
}

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
    m_merchant = new QComboBox(card);
    m_merchant->setEditable(false);                // pick from the list only
    m_merchant->setStyleSheet(comboStyle());
    m_merchant->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

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
        fillMerchants();
        updateState();
    });
    connect(m_merchant, &QComboBox::currentIndexChanged, this, &PaymentDialog::updateState);

    m_mode->setCurrentIndex(CashIndex);
    fillMerchants();
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
    return (needsMerchant() && m_merchant->currentIndex() >= 0) ? m_merchant->currentText() : QString();
}

bool PaymentDialog::needsMerchant() const
{
    return m_mode->currentIndex() != CashIndex;
}

void PaymentDialog::fillMerchants()
{
    const QSignalBlocker blocker(m_merchant);
    m_merchant->clear();
    if (m_mode->currentIndex() == CardIndex) {
        m_merchant->addItems(kBanks);
        m_merchant->setPlaceholderText("Select bank");
    } else if (m_mode->currentIndex() == WalletIndex) {
        m_merchant->addItems(kWallets);
        m_merchant->setPlaceholderText("Select wallet");
    }
    m_merchant->setCurrentIndex(-1);               // nothing chosen yet: the cashier must pick one
}

void PaymentDialog::updateState()
{
    m_merchant->setVisible(needsMerchant());
    m_confirm->setEnabled(!needsMerchant() || m_merchant->currentIndex() >= 0);
}
