#pragma once

#include <QDialog>

class QKeyEvent;

// "STORE POLICY" popup shown when a cashier tries to take every product out of an edited
// order. A friendly card with a big angry face, not an error message.
// Enter, Escape and the button all close it.
class StorePolicyDialog : public QDialog {
    Q_OBJECT
public:
    explicit StorePolicyDialog(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
