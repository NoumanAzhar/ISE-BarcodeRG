#pragma once

#include <QDialog>

class Bill;

// The bill as a receipt on white paper. Enter, Escape and Done close it.
class BillDialog : public QDialog {
    Q_OBJECT
public:
    explicit BillDialog(const Bill& bill, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
