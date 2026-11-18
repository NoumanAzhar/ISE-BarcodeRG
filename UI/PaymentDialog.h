#pragma once

#include "order.h"

#include <QDialog>
#include <QString>

class NeuButton;
class NeuLineEdit;
class NeuSegmented;

// Asks how the bill is paid. Accepted only when the payment is confirmed.
class PaymentDialog : public QDialog {
    Q_OBJECT
public:
    explicit PaymentDialog(double totalPayable, QWidget* parent = nullptr);

    pos::PaymentMode mode() const;      // Cash, Card or Wallet
    QString merchant() const;           // trimmed; empty for Cash

private:
    bool needsMerchant() const;
    void updateState();                 // merchant field visibility, Confirm enabled

    NeuSegmented* m_mode = nullptr;
    NeuLineEdit* m_merchant = nullptr;
    NeuButton* m_confirm = nullptr;
};
