#pragma once

#include "PageBase.h"

#include <QElapsedTimer>
#include <QString>

namespace pos { class AppServices; }

class Banner;
class CameraScannerWidget;
class NeuButton;
class NeuLineEdit;
class OrderItemList;
class QLabel;

// Cashier's "New Order": scan or type barcodes, edit quantities, Generate Bill.
class OrderPage : public PageBase {
    Q_OBJECT
public:
    static constexpr const char* kTitle = "New Order";   // the rail item that shows this page
    static constexpr const char* kEditTitle = "Edit Order";   // shown instead while a paid order is edited

    OrderPage(pos::AppServices& services, QWidget* parent = nullptr);

    bool canLeave() override;
    void onActivated() override;
    void onDeactivated() override;
    void resetState() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* buildScanCard();
    QWidget* buildOrderCard();

    void addBarcode(const QString& code);
    void onCameraScanned(const QString& digits);
    void showDescription();
    void clearItems();
    void discardEditChanges();      // paid order: back to the saved items
    void holdOrder();
    void showHoldList();
    void syncMode();                // title, buttons and fields follow the draft (new, resumed, edited)
    void generateBill();
    void resetFields();
    void refreshAll();
    void updateTotals();

    pos::AppServices& m_services;
    bool m_active = false;

    // left card
    CameraScannerWidget* m_camera = nullptr;
    Banner* m_banner = nullptr;
    NeuLineEdit* m_code = nullptr;

    // right card
    QLabel* m_title = nullptr;
    NeuButton* m_holdListBtn = nullptr;
    NeuButton* m_holdBtn = nullptr;
    NeuButton* m_billBtn = nullptr;
    NeuButton* m_clearBtn = nullptr;
    QLabel* m_seasonKey = nullptr;
    QLabel* m_season = nullptr;
    QString m_displayTitle;         // last title announced with displayTitleChanged()
    QString m_shownOrderId;         // the draft whose customer was last copied into the fields
    NeuLineEdit* m_name = nullptr;
    NeuLineEdit* m_contact = nullptr;
    OrderItemList* m_list = nullptr;
    QLabel* m_subtotal = nullptr;
    QLabel* m_tax = nullptr;
    QLabel* m_discount = nullptr;
    QLabel* m_payable = nullptr;
    NeuButton* m_descriptionBtn = nullptr;

    QString m_lastScan;             // camera repeats a barcode while it stays in view
    QElapsedTimer m_lastScanTime;
};
