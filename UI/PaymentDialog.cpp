#include "PaymentDialog.h"

#include "CheckoutService.h"
#include "Data_Files.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {

enum ModeIndex { CashIndex, CardIndex, WalletIndex };

const QStringList kBanks = { "Meezan Bank", "HBL", "MCB", "UBL", "ABL" };
const QStringList kWallets = { "JazzCash", "EasyPaisa", "NayaPay" };

constexpr double kCent = 0.005;       // amounts are compared to the nearest paisa

} // namespace

PaymentDialog::PaymentDialog(const PaymentQuote& quote, QWidget* parent)
    : QDialog(parent), m_quote(quote)
{
    setWindowTitle("Payment");
    setMinimumWidth(440);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);       // the card already holds its shadow room
    layout->setSpacing(2);

    NeuFrame* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(6);

    m_total = makeLabel(QString(), "totalValue", card);
    m_note = makeLabel(QString(), "muted", card);
    if (m_quote.seasonalPercent > 0)
        m_note->setText(QStringLiteral("Includes the %1% season offer.")
                            .arg(QLocale::c().toString(m_quote.seasonalPercent, 'f',
                                                       m_quote.seasonalPercent == int(m_quote.seasonalPercent) ? 0 : 2)));
    m_note->setVisible(m_quote.seasonalPercent > 0);

    m_mode = new NeuSegmented(QStringList{ "Cash", "Card", "Wallet" }, card);

    m_merchant = new QComboBox(card);
    m_merchant->setEditable(false);                // pick from the list only
    m_merchant->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_special = new NeuLineEdit(card);
    m_special->setPlaceholderText("0");
    m_special->setMaxLength(5);
    m_special->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,2}([.][0-9]{0,2})?"), m_special));

    m_received = new NeuLineEdit(card);
    m_received->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,9}([.][0-9]{0,2})?"), m_received));
    m_receivedField = makeField("Amount received", m_received, card);

    auto* amounts = new QHBoxLayout;
    amounts->setSpacing(2);
    amounts->addWidget(makeField(QStringLiteral("Special discount (up to %1%)")
                                     .arg(QLocale::c().toString(pos::kMaxSpecialPercent, 'f', 0)),
                                 m_special, card), 1);
    amounts->addWidget(m_receivedField, 1);

    m_change = makeLabel(QString(), "hint", card);

    cardLayout->addWidget(makeLabel("TO PAY", "sectionTitle", card));
    cardLayout->addWidget(m_total);
    cardLayout->addWidget(m_note);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(makeLabel("PAYMENT", "sectionTitle", card));
    cardLayout->addWidget(m_mode);
    cardLayout->addWidget(m_merchant);
    cardLayout->addSpacing(4);
    cardLayout->addLayout(amounts);
    cardLayout->addWidget(m_change);

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
        refresh();
    });
    connect(m_merchant, &QComboBox::currentIndexChanged, this, &PaymentDialog::refresh);
    connect(m_special, &QLineEdit::textChanged, this, &PaymentDialog::refresh);
    connect(m_received, &QLineEdit::textEdited, this, [this]() {
        m_receivedEdited = true;
        refresh();
    });

    m_mode->setCurrentIndex(CashIndex);
    fillMerchants();
    refresh();
}

// ------------------------------------------------------------------ values

bool PaymentDialog::isCash() const
{
    return m_mode->currentIndex() == CashIndex;
}

bool PaymentDialog::needsMerchant() const
{
    return !isCash();
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

bool PaymentDialog::parseSpecial(double& percent) const
{
    percent = 0;
    const QString text = m_special->text().trimmed();
    if (text.isEmpty())
        return true;
    bool ok = false;
    const double value = QLocale::c().toDouble(text, &ok);
    if (!ok || value < 0 || value > pos::kMaxSpecialPercent)
        return false;
    percent = value;
    return true;
}

double PaymentDialog::specialPercent() const
{
    double percent = 0;
    return parseSpecial(percent) ? percent : 0.0;
}

double PaymentDialog::payable() const
{
    return m_quote.payableFor ? m_quote.payableFor(specialPercent()) : 0.0;
}

double PaymentDialog::amountReceived() const
{
    if (!isCash())
        return 0.0;                               // card and wallet pay exactly
    bool ok = false;
    const double received = QLocale::c().toDouble(m_received->text().trimmed(), &ok);
    return ok ? received : 0.0;
}

// ------------------------------------------------------------------ screen

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

void PaymentDialog::refresh()
{
    double percent = 0;
    const bool specialOk = parseSpecial(percent);
    m_special->setState(specialOk ? QString() : QStringLiteral("invalid"));

    const double toPay = specialOk ? m_quote.payableFor(percent) : m_quote.payableFor(0);
    m_total->setText(formatPkr(toPay));

    m_merchant->setVisible(needsMerchant());
    m_receivedField->setVisible(isCash());

    // The received field follows the amount to pay until the cashier types in it.
    if (isCash() && !m_receivedEdited) {
        const QSignalBlocker blocker(m_received);
        // Cash is taken in whole notes (smallest note PKR 10); the rest is change.
        m_received->setText(QLocale::c().toString(pos::cashToCollect(toPay), 'f', 0));
    }

    bool enoughCash = true;
    QString changeText;
    QString changeState;
    if (!specialOk) {
        changeText = QStringLiteral("The special discount can be up to %1%.")
                         .arg(QLocale::c().toString(pos::kMaxSpecialPercent, 'f', 0));
        changeState = "invalid";
    } else if (isCash()) {
        bool ok = false;
        const double received = QLocale::c().toDouble(m_received->text().trimmed(), &ok);
        if (!ok) {
            enoughCash = false;
            changeText = "Enter the amount received.";
        } else if (received + kCent < toPay) {
            enoughCash = false;
            changeText = QStringLiteral("%1 short.").arg(formatPkr(toPay - received));
            changeState = "invalid";
        } else {
            changeText = QStringLiteral("Change to return: %1").arg(formatPkr(std::max(0.0, received - toPay)));
            changeState = "valid";
        }
    }
    m_change->setText(changeText);
    m_change->setProperty("state", changeState);
    repolish(m_change);
    m_change->setVisible(!changeText.isEmpty());

    m_confirm->setEnabled(specialOk && enoughCash && (!needsMerchant() || m_merchant->currentIndex() >= 0));
}
