#pragma once

#include "order.h"

#include <QDialog>
#include <QString>

class NeuButton;
class NeuSegmented;
class QComboBox;

// Asks how the bill is paid. Accepted only when the payment is confirmed.
class PaymentDialog : public QDialog {
    Q_OBJECT
public:
    explicit PaymentDialog(double totalPayable, QWidget* parent = nullptr);

    pos::PaymentMode mode() const;      // Cash, Card or Wallet
    QString merchant() const;           // the bank or wallet picked from the list; empty for Cash

private:
    bool needsMerchant() const;
    void updateState();                 // list visibility and contents, Confirm enabled
    void fillMerchants();               // banks for Card, wallets for Wallet

    NeuSegmented* m_mode = nullptr;
    QComboBox* m_merchant = nullptr;    // drop-down only: the cashier cannot type a name
    NeuButton* m_confirm = nullptr;
};
