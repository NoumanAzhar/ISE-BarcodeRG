#pragma once

#include "PageBase.h"

#include "Product.h"

#include <QPixmap>
#include <QString>

#include <vector>

namespace pos { class AppServices; }

class Banner;
class NeuButton;
class NeuLineEdit;
class ProductDetailsView;
class QLabel;
class QStackedWidget;
class QTableWidget;

// Manager's product list: search, specs, barcode image and "Assign barcodes".
class ProductsPage : public PageBase {
    Q_OBJECT
public:
    ProductsPage(pos::AppServices& services, QWidget* parent = nullptr);

    void onActivated() override;       // reload the list (stock changes after every sale)
    void onDeactivated() override;
    void resetState() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;     // theme switch: recolour the out-of-stock numbers

private:
    QWidget* buildListSide();
    QWidget* buildDetailsCard();

    void reload();
    void applyFilter();
    void addRow(const pos::Product& product);
    void updateStockColors();
    int nameIndent() const;          // pixels that put the Name values exactly under the "Name" header
    void updateNameIndent();
    void selectById(const QString& id);
    QString selectedId() const;
    void showSelected();
    void showBarcodeImage();
    void assignBarcodes();
    void addProduct();
    void editSelected();

    pos::AppServices& m_services;
    std::vector<pos::Product> m_products;

    NeuLineEdit* m_search = nullptr;
    NeuButton* m_addBtn = nullptr;
    QStackedWidget* m_listStack = nullptr;     // 0 = table, 1 = empty text
    QTableWidget* m_table = nullptr;
    QLabel* m_emptyLabel = nullptr;

    ProductDetailsView* m_details = nullptr;
    QStackedWidget* m_imageStack = nullptr;    // 0 = barcode on paper, 1 = "No barcode image"
    QLabel* m_image = nullptr;
    Banner* m_banner = nullptr;
    NeuButton* m_editBtn = nullptr;

    QPixmap m_barcodePixmap;
    bool m_active = false;
    int m_pressedSelectedRow = -1;   // row that was already selected when the mouse went down
};
