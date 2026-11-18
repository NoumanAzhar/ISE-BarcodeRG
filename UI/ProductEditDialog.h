#pragma once

#include "Product.h"

#include <QDialog>
#include <optional>
#include <string>

namespace pos { class AppServices; }

class Banner;
class NeuButton;
class NeuLineEdit;
class NeuSegmented;

class ProductEditDialog : public QDialog {
    Q_OBJECT
public:
    // If product is provided, opens in Edit mode; otherwise opens in Add mode.
    explicit ProductEditDialog(pos::AppServices& services,
                               std::optional<pos::Product> product = std::nullopt,
                               QWidget* parent = nullptr);

    std::string savedProductId() const { return m_savedId; }

private slots:
    void onSave();
    void generateRandomBarcode();

private:
    void populateFields();
    void clearErrors();

    pos::AppServices& m_services;
    std::optional<pos::Product> m_original;
    std::string m_savedId;

    NeuLineEdit* m_name = nullptr;
    NeuLineEdit* m_acronym = nullptr;
    NeuSegmented* m_unit = nullptr;
    NeuLineEdit* m_price = nullptr;
    NeuLineEdit* m_tax = nullptr;
    NeuLineEdit* m_discount = nullptr;
    NeuLineEdit* m_stock = nullptr;
    NeuLineEdit* m_barcode = nullptr;
    NeuButton* m_randomBtn = nullptr;
    Banner* m_banner = nullptr;
};

