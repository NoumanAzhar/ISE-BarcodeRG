#pragma once

#include "PageBase.h"

#include "Inventory.h"

#include <vector>

namespace pos { class AppServices; }

class Banner;
class DataTable;
class NeuButton;
class NeuLineEdit;
class NeuSegmented;
class QLabel;
class QWidget;

// Manager: stock of every product, with set / add stock.
class InventoryPage : public PageBase {
    Q_OBJECT
public:
    explicit InventoryPage(pos::AppServices& services, QWidget* parent = nullptr);

    void onActivated() override;       // stock changes after every sale
    void resetState() override;

private:
    QWidget* buildListSide();
    QWidget* buildEditor();

    void reload();
    void applyFilter();
    void showSelected();
    void apply(bool addToStock);
    const pos::InventoryRow* selectedRow() const;

    pos::AppServices& m_services;
    std::vector<pos::InventoryRow> m_rows;

    NeuLineEdit* m_search = nullptr;
    NeuSegmented* m_filter = nullptr;
    DataTable* m_table = nullptr;

    QLabel* m_placeholder = nullptr;
    QWidget* m_editor = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_sub = nullptr;
    QLabel* m_stock = nullptr;
    QLabel* m_stockNote = nullptr;
    NeuLineEdit* m_amount = nullptr;
    NeuButton* m_setBtn = nullptr;
    NeuButton* m_addBtn = nullptr;
    Banner* m_banner = nullptr;
};
