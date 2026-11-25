#pragma once

#include "order.h"

#include <QDialog>
#include <QString>

#include <functional>

class NeuButton;
class NeuLineEdit;
class NeuSegmented;
class QComboBox;
class QLabel;
class QWidget;

// What the dialog needs to know about the bill it is asking payment for.
struct PaymentQuote {
    double seasonalPercent = 0;                    // season offer already in the price (0 = none)
    std::function<double(double)> payableFor;      // amount to pay after a special discount of x percent
};

// Asks how the bill is paid: mode (and bank or wallet), an optional special
// discount, and for cash the amount received, with the change worked out live.
// Accepted only when the payment is complete.
class PaymentDialog : public QDialog {
    Q_OBJECT
public:
    explicit PaymentDialog(const PaymentQuote& quote, QWidget* parent = nullptr);

    pos::PaymentMode mode() const;      // Cash, Card or Wallet
    QString merchant() const;           // the bank or wallet picked from the list; empty for Cash
    double specialPercent() const;      // 0 when none
    double amountReceived() const;      // cash: the amount typed (starts at the payable rounded up to a multiple of 10); 0 for card and wallet

private:
    bool needsMerchant() const;
    bool isCash() const;
    bool parseSpecial(double& percent) const;       // false when the text is not allowed
    double payable() const;
    void fillMerchants();               // banks for Card, wallets for Wallet
    void refresh();                     // totals, change, field visibility, Confirm enabled

    PaymentQuote m_quote;

    QLabel* m_total = nullptr;
    QLabel* m_note = nullptr;
    NeuSegmented* m_mode = nullptr;
    QComboBox* m_merchant = nullptr;    // drop-down only: the cashier cannot type a name
    NeuLineEdit* m_special = nullptr;
    NeuLineEdit* m_received = nullptr;
    QWidget* m_receivedField = nullptr;
    QLabel* m_change = nullptr;
    NeuButton* m_confirm = nullptr;
    bool m_receivedEdited = false;      // false: the field follows the amount to pay
};
