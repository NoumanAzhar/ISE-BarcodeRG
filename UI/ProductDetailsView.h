#pragma once

#include "Product.h"

#include <QString>
#include <QWidget>

#include <optional>

class QGridLayout;
class QLabel;
class QStackedLayout;

// Product specification as two aligned columns (no card of its own: the page
// puts it inside a card). Used by the Read page, the Products page and ProductDialog.
class ProductDetailsView : public QWidget {
    Q_OBJECT
public:
    explicit ProductDetailsView(QWidget* parent = nullptr);

    // categoryName: nullopt hides the Category row, an empty text shows "none".
    void showProduct(const pos::Product& product,
                     const std::optional<QString>& categoryName = std::nullopt);
    void showMessage(const QString& text);     // empty state: one centred muted line
    void clear();                              // blank

private:
    enum Page { DetailsPage, MessagePage, BlankPage };

    QWidget* buildDetailsPage();
    QLabel* addRow(QGridLayout* grid, const QString& label, QLabel** keyOut = nullptr);

    QStackedLayout* m_stack = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_acronym = nullptr;
    QLabel* m_categoryKey = nullptr;
    QLabel* m_category = nullptr;
    QLabel* m_id = nullptr;
    QLabel* m_barcode = nullptr;
    QLabel* m_unit = nullptr;
    QLabel* m_price = nullptr;
    QLabel* m_tax = nullptr;
    QLabel* m_discount = nullptr;
    QLabel* m_available = nullptr;
    QLabel* m_message = nullptr;
};
