#pragma once

#include "PageBase.h"

#include "Inventory.h"

#include <QPixmap>

#include <vector>

namespace pos { class AppServices; }

class Banner;
class DataTable;
class NeuButton;
class NeuLineEdit;
class NeuSegmented;
class ProductDetailsView;
class QLabel;
class QStackedWidget;
class QWidget;

class InventoryPage : public PageBase {
    Q_OBJECT
public:
    explicit InventoryPage(pos::AppServices& services, QWidget* parent = nullptr);

    void onActivated() override;
    void resetState() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* buildListSide();
    QWidget* buildDetailsCard();

    void reload();
    void applyFilter();
    void showSelected();
    void showBarcodeImage();
    void addProduct();
    void editSelected();
    void assignBarcodes();
    const pos::InventoryRow* selectedRow() const;

    pos::AppServices& m_services;
    std::vector<pos::InventoryRow> m_rows;

    NeuLineEdit* m_search = nullptr;
    NeuSegmented* m_filter = nullptr;
    NeuButton* m_addBtn = nullptr;
    DataTable* m_table = nullptr;

    ProductDetailsView* m_details = nullptr;
    QStackedWidget* m_imageStack = nullptr;
    QLabel* m_image = nullptr;
    Banner* m_banner = nullptr;
    NeuButton* m_editBtn = nullptr;
    QPixmap m_barcodePixmap;
};
