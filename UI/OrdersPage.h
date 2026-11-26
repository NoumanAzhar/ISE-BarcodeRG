#pragma once

#include "PageBase.h"

#include "order.h"

#include <QHash>
#include <QString>

#include <functional>
#include <vector>

namespace pos { class AppServices; }

class Banner;
class DataTable;
class NeuButton;
class NeuLineEdit;
class NeuSegmented;
class QLabel;
class QWidget;

// Cashier: look up saved orders, resume held ones, edit paid ones (within the
// edit window and after approval) and view bills.
// Manager: approve or decline edit requests, archive orders and view bills.
enum class OrdersMode { Cashier, Manager };

class OrdersPage : public PageBase {
    Q_OBJECT
public:
    static constexpr const char* kTitle = "Orders";   // the rail item that shows this page

    OrdersPage(pos::AppServices& services, OrdersMode mode, QWidget* parent = nullptr);

    void onActivated() override;       // orders change after every sale
    void resetState() override;

private:
    QWidget* buildListSide();
    QWidget* buildDetails();

    void reload();
    void applyFilter();
    void showSelected();
    void configureActions(const pos::Order& order);
    const pos::Order* selectedOrder() const;
    bool can(int action) const;        // pos::Action, kept as int so this header stays light

    void resume(const QString& orderId);
    void openForEdit(const QString& orderId);
    void requestApproval(const QString& orderId);
    void decide(const QString& orderId, bool approve);
    void archive(const QString& orderId);
    void viewBill(const QString& orderId);

    pos::AppServices& m_services;
    OrdersMode m_mode;

    std::vector<pos::Order> m_orders;          // newest first
    QHash<QString, int> m_access;              // paid order id -> pos::EditAccess

    // list side
    NeuLineEdit* m_search = nullptr;
    NeuSegmented* m_filter = nullptr;
    DataTable* m_table = nullptr;
    Banner* m_inbox = nullptr;                 // manager: "n requests are waiting"

    // details side
    QLabel* m_placeholder = nullptr;
    QWidget* m_details = nullptr;
    QLabel* m_heading = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_customer = nullptr;
    QLabel* m_contact = nullptr;
    QLabel* m_when = nullptr;
    QLabel* m_editedCaption = nullptr;         // shown only for an order that was edited
    QLabel* m_edited = nullptr;
    QLabel* m_payment = nullptr;
    QLabel* m_total = nullptr;
    QLabel* m_items = nullptr;
    QLabel* m_note = nullptr;
    Banner* m_banner = nullptr;
    NeuButton* m_primaryBtn = nullptr;
    NeuButton* m_secondBtn = nullptr;
    NeuButton* m_billBtn = nullptr;
    std::function<void()> m_onPrimary;
    std::function<void()> m_onSecond;
};
