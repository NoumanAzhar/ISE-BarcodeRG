#pragma once

#include "Product.h"

#include <QDialog>
#include <QString>

#include <optional>
#include <string>

namespace pos { class AppServices; }

class Banner;
class NeuButton;
class NeuLineEdit;
class NeuSegmented;
class QComboBox;

// Add a product or edit one. A new product gets its barcode automatically
// from its category (category ID + a 4-digit serial); an existing barcode
// never changes, so the category of a product that has one is locked.
class ProductEditDialog : public QDialog {
    Q_OBJECT
public:
    // If product is provided, opens in Edit mode; otherwise opens in Add mode.
    explicit ProductEditDialog(pos::AppServices& services,
                               std::optional<pos::Product> product = std::nullopt,
                               QWidget* parent = nullptr);

    std::string savedProductId() const { return m_savedId; }
    // A warning to show after saving (for example the barcode could not be
    // created). Empty when everything went well.
    QString savedNotice() const { return m_notice; }

private:
    void fillCategories();
    void populateFields();
    void clearErrors();
    void onSave();

    pos::AppServices& m_services;
    std::optional<pos::Product> m_original;
    std::string m_savedId;
    QString m_notice;
    bool m_sold = false;       // Edit mode: the product appears in a saved order

    NeuLineEdit* m_name = nullptr;
    NeuLineEdit* m_acronym = nullptr;
    QComboBox* m_category = nullptr;
    NeuSegmented* m_unit = nullptr;
    NeuLineEdit* m_price = nullptr;
    NeuLineEdit* m_tax = nullptr;
    NeuLineEdit* m_discount = nullptr;
    NeuLineEdit* m_stock = nullptr;
    NeuLineEdit* m_barcode = nullptr;       // Edit mode only: shown locked
    NeuButton* m_saveBtn = nullptr;
    Banner* m_banner = nullptr;
};
