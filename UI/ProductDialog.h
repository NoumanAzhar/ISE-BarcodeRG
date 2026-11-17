#pragma once

#include "Product.h"

#include <QDialog>

// Shows one product's specification. Enter, Escape and Close all close it.
class ProductDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProductDialog(const pos::Product& product, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
