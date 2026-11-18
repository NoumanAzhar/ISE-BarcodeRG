#pragma once

#include "Product.h"
#include "order.h"

#include <QHash>
#include <QString>
#include <QWidget>

namespace pos { class AppServices; }

class NeuFrame;
class NeuLineEdit;
class QLabel;
class QScrollArea;
class QVBoxLayout;

// The items of the order being recorded, with quantity editing and removal.
// Shows services.order().current().items and edits them through OrderService.
class OrderItemList : public QWidget {
    Q_OBJECT
public:
    explicit OrderItemList(pos::AppServices& services, QWidget* parent = nullptr);

    void refresh();                          // rebuild all rows from the draft order
    void pulse(const QString& productId);    // brief highlight of that row (about 600 ms)
    QString selectedProductId() const;       // empty when no row is selected

signals:
    void changed();                          // quantity edited or item removed: recalculate totals
    void selectionChanged(const QString& productId);   // empty when selection cleared
    void notice(const QString& text);        // e.g. "Not enough stock (max 5)"

private:
    struct Row {
        NeuFrame* frame = nullptr;
        NeuLineEdit* edit = nullptr;
        QLabel* total = nullptr;
    };

    void clearRows();
    void addRow(const pos::OrderItem& item, const pos::Product& product);
    void select(const QString& id);
    void styleRow(const QString& id, bool pulsing = false);
    double quantityOf(const QString& id) const;
    void step(const QString& id, int direction);
    void commit(const QString& id, double quantity);
    void showQuantity(const QString& id, double quantity);
    void remove(const QString& id);

    pos::AppServices& m_services;

    QLabel* m_empty = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_content = nullptr;
    QVBoxLayout* m_rowLayout = nullptr;

    QHash<QString, Row> m_rowWidgets;
    QHash<QString, pos::Product> m_products;   // cached per refresh()
    QString m_selectedId;
};
