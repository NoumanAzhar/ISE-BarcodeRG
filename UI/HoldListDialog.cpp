#include "HoldListDialog.h"

#include "AppServices.h"
#include "DataTable.h"
#include "Format.h"
#include "Neu.h"
#include "OrderTotals.h"
#include "PosActions.h"
#include "Widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

HoldListDialog::HoldListDialog(pos::AppServices& services, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Orders on hold");
    setMinimumSize(560, 420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(2);

    NeuFrame* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(4);

    auto* hint = makeLabel("Pick an order to continue it. The order you are working on stays on hold.",
                           "muted", card);
    hint->setWordWrap(true);

    m_table = new DataTable(card);
    m_table->setColumns({
        { "Order", DataTable::Align::Left, false },
        { "Customer", DataTable::Align::Left, true },
        { "Items", DataTable::Align::Right, false },
        { "Total", DataTable::Align::Right, false },
    });
    m_table->setEmptyText("No orders are on hold.");

    const double season = PosActions::seasonPercentToday(services);
    for (const pos::Order& order : services.hold().list()) {
        const pos::OrderTotals totals = pos::computeTotals(order, services.products(), season);
        m_table->addRow({ qs(order.id), qs(order.customerName),
                          QString::number(order.items.size()), formatPkr(totals.payable) },
                        qs(order.id));
    }

    cardLayout->addWidget(makeLabel("ORDERS ON HOLD", "sectionTitle", card));
    cardLayout->addWidget(hint);
    cardLayout->addWidget(m_table, 1);

    auto* cancel = new NeuButton("Cancel", NeuButton::Variant::Normal, this);
    cancel->setAutoDefault(false);
    m_resumeBtn = new NeuButton("Resume order", NeuButton::Variant::Primary, this);
    m_resumeBtn->setDefault(true);
    m_resumeBtn->setEnabled(false);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    buttons->addStretch(1);
    buttons->addWidget(cancel);
    buttons->addWidget(m_resumeBtn);

    layout->addWidget(card, 1);
    layout->addLayout(buttons);

    connect(cancel, &NeuButton::clicked, this, &QDialog::reject);
    connect(m_resumeBtn, &NeuButton::clicked, this, [this]() { choose(m_table->selectedKey()); });
    connect(m_table, &DataTable::activated, this, &HoldListDialog::choose);
    connect(m_table, &DataTable::selectionChanged, this,
            [this](const QString& key) { m_resumeBtn->setEnabled(!key.isEmpty()); });

    // One order on hold: it is already the obvious choice.
    if (m_table->rowCount() == 1)
        m_table->selectKey(qs(services.hold().list().front().id));
}

void HoldListDialog::choose(const QString& orderId)
{
    if (orderId.isEmpty())
        return;
    m_chosen = orderId;
    accept();
}
