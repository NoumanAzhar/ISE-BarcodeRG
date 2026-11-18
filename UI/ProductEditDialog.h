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
    bool wasDeleted() const { return m_deleted; }     // Edit mode: the product was deleted

private slots:
    void onSave();
    void onDelete();
    void generateRandomBarcode();

private:
    void populateFields();
    void clearErrors();
    bool confirmNewProduct(const QString& name, const QString& acronym, bool counted);

    pos::AppServices& m_services;
    std::optional<pos::Product> m_original;
    std::string m_savedId;
    bool m_sold = false;       // Edit mode: the product appears in a saved order
    bool m_deleted = false;

    NeuLineEdit* m_name = nullptr;
    NeuLineEdit* m_acronym = nullptr;
    NeuSegmented* m_unit = nullptr;
    NeuLineEdit* m_price = nullptr;
    NeuLineEdit* m_tax = nullptr;
    NeuLineEdit* m_discount = nullptr;
    NeuLineEdit* m_stock = nullptr;
    NeuLineEdit* m_barcode = nullptr;
    NeuButton* m_randomBtn = nullptr;       // Add mode only
    NeuButton* m_deleteBtn = nullptr;       // Edit mode only
    Banner* m_banner = nullptr;
};

