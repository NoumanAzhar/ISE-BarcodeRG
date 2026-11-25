#include "OrdersPage.h"

#include "AppServices.h"
#include "BillDialog.h"
#include "Data_Files.h"
#include "DataTable.h"
#include "Format.h"
#include "Neu.h"
#include "OrderPage.h"
#include "PosActions.h"
#include "Widgets.h"
#include "access.h"
#include "bill_pdf.h"
#include "order_edit.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace {

enum Filter { ShowAll, ShowOnHold, ShowPaid, ShowApproved, ShowWaiting };

constexpr int kMaxItemLines = 6;

struct StatusInfo {
    QString text;
    DataTable::Tone tone;
    const char* kind;          // banner colour: info, success, warning
};

StatusInfo statusOf(const pos::Order& order, int access)
{
    switch (order.status) {
    case pos::OrderStatus::OnHold:
        return { "On hold", DataTable::Tone::Warn, "warning" };
    case pos::OrderStatus::InProgress:
        return { "In progress", DataTable::Tone::Muted, "info" };
    case pos::OrderStatus::Archived:
        return { "Archived", DataTable::Tone::Muted, "info" };
    case pos::OrderStatus::Paid:
        break;
    }
    if (access == int(pos::EditAccess::PendingApproval))
        return { "Waiting for approval", DataTable::Tone::Warn, "warning" };
    if (access == int(pos::EditAccess::Allowed))                   // approved, not edited yet
        return { "Approved", DataTable::Tone::Warn, "warning" };
    if (access == int(pos::EditAccess::AlreadyEdited))             // the one edit is done
        return { "Edited", DataTable::Tone::Muted, "info" };
    return { "Paid", DataTable::Tone::Good, "success" };
}

bool isSettled(const pos::Order& order)
{
    return order.status == pos::OrderStatus::Paid || order.status == pos::OrderStatus::Archived;
}

QString paymentText(const pos::Order& order)
{
    switch (order.paymentMode) {
    case pos::PaymentMode::Cash:   return "Cash";
    case pos::PaymentMode::Card:   return order.paymentMerchant.empty() ? QString("Card")
                                              : "Card " + QString(kMiddleDot) + " " + qs(order.paymentMerchant);
    case pos::PaymentMode::Wallet: return order.paymentMerchant.empty() ? QString("Wallet")
                                              : "Wallet " + QString(kMiddleDot) + " " + qs(order.paymentMerchant);
    default:                       return "-";
    }
}

QLabel* addDetail(QGridLayout* grid, const QString& caption, QWidget* parent, QLabel** captionOut = nullptr)
{
    const int row = grid->rowCount();
    auto* value = makeLabel(QString(), "fieldLabel", parent);
    value->setWordWrap(true);
    auto* captionLabel = makeLabel(caption, "muted", parent);
    grid->addWidget(captionLabel, row, 0, Qt::AlignTop);
    grid->addWidget(value, row, 1, Qt::AlignTop);
    if (captionOut)
        *captionOut = captionLabel;
    return value;
}

} // namespace

OrdersPage::OrdersPage(pos::AppServices& services, OrdersMode mode, QWidget* parent)
    : PageBase(parent), m_services(services), m_mode(mode)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room
    root->addWidget(buildListSide(), 3);
    root->addWidget(buildDetails(), 2);

    showSelected();
}

bool OrdersPage::can(int action) const
{
    const pos::Role role = m_mode == OrdersMode::Manager ? pos::Role::Manager : pos::Role::Cashier;
    return pos::isAllowed(role, static_cast<pos::Action>(action));
}

// ---------------------------------------------------------------- building

QWidget* OrdersPage::buildListSide()
{
    auto* side = new QWidget(this);
    auto* layout = new QVBoxLayout(side);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_search = new NeuLineEdit(side);
    m_search->setPlaceholderText("Search order number or customer");
    m_filter = new NeuSegmented(QStringList{ "All", "On hold", "Paid", "Approved", "Waiting" }, side);
    m_filter->setMinimumWidth(470);

    auto* top = new QHBoxLayout;
    top->setSpacing(2);
    top->addWidget(m_search, 1);
    top->addWidget(m_filter);

    m_inbox = new Banner(side);

    m_table = new DataTable(side);
    m_table->setColumns({
        { "Order", DataTable::Align::Left, false },
        { "Date", DataTable::Align::Left, false },
        { "Customer", DataTable::Align::Left, true },
        { "Status", DataTable::Align::Left, false },
        { "Total", DataTable::Align::Right, false },
    });

    layout->addLayout(top);
    layout->addWidget(m_inbox);
    layout->addWidget(m_table, 1);

    connect(m_search, &QLineEdit::textChanged, this, &OrdersPage::applyFilter);
    connect(m_filter, &NeuSegmented::currentChanged, this, &OrdersPage::applyFilter);
    connect(m_table, &DataTable::selectionChanged, this, &OrdersPage::showSelected);
    return side;
}

QWidget* OrdersPage::buildDetails()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(6);

    m_placeholder = makeLabel("Select an order to see its details.", "muted", card);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);

    m_details = new QWidget(card);
    auto* details = new QVBoxLayout(m_details);
    details->setContentsMargins(0, 0, 0, 0);
    details->setSpacing(8);

    m_heading = makeLabel(QString(), "cardTitle", m_details);
    m_status = new QLabel(m_details);                 // coloured like a banner
    m_status->setObjectName("banner");
    m_status->setProperty("kind", "info");

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(1, 1);
    m_customer = addDetail(grid, "Customer", m_details);
    m_contact = addDetail(grid, "Contact", m_details);
    m_when = addDetail(grid, "Paid on", m_details);
    m_edited = addDetail(grid, "Edited on", m_details, &m_editedCaption);
    m_payment = addDetail(grid, "Payment", m_details);
    m_total = addDetail(grid, "Total", m_details);
    m_items = addDetail(grid, "Items", m_details);

    m_note = makeLabel(QString(), "muted", m_details);
    m_note->setWordWrap(true);

    m_banner = new Banner(m_details);

    m_billBtn = new NeuButton("View bill", NeuButton::Variant::Normal, m_details);
    m_secondBtn = new NeuButton(QString(), NeuButton::Variant::Normal, m_details);
    m_primaryBtn = new NeuButton(QString(), NeuButton::Variant::Primary, m_details);
    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    buttons->addWidget(m_billBtn);
    buttons->addStretch(1);
    buttons->addWidget(m_secondBtn);
    buttons->addWidget(m_primaryBtn);

    details->addWidget(m_heading);
    details->addWidget(m_status, 0, Qt::AlignLeft);
    details->addLayout(grid);
    details->addWidget(m_note);
    details->addStretch(1);
    details->addWidget(m_banner);
    details->addLayout(buttons);

    layout->addWidget(m_placeholder, 1);
    layout->addWidget(m_details, 1);

    // The handlers are copied before they run: they reload the page, which replaces them.
    connect(m_primaryBtn, &NeuButton::clicked, this, [this]() { const auto run = m_onPrimary; if (run) run(); });
    connect(m_secondBtn, &NeuButton::clicked, this, [this]() { const auto run = m_onSecond; if (run) run(); });
    connect(m_billBtn, &NeuButton::clicked, this, [this]() {
        if (const pos::Order* order = selectedOrder())
            viewBill(qs(order->id));
    });
    return card;
}

// ------------------------------------------------------------- page hooks

void OrdersPage::onActivated()
{
    reload();
}

void OrdersPage::resetState()
{
    m_search->clear();
    m_filter->setCurrentIndex(ShowAll);
    m_table->clearSelection();
    m_banner->clearMessage();
}

// -------------------------------------------------------------------- list

void OrdersPage::reload()
{
    m_orders = m_services.orders().all();
    std::sort(m_orders.begin(), m_orders.end(),
              [](const pos::Order& a, const pos::Order& b) { return a.id > b.id; });   // newest first

    m_access.clear();
    int waiting = 0;
    int approved = 0;                          // approved by the manager, not edited yet
    for (const pos::Order& order : m_orders) {
        if (order.status != pos::OrderStatus::Paid)
            continue;
        const pos::EditAccess access = m_services.orderEdit().check(order.id).access;
        m_access.insert(qs(order.id), int(access));
        if (access == pos::EditAccess::PendingApproval)
            ++waiting;
        else if (access == pos::EditAccess::Allowed)
            ++approved;
    }

    if (m_mode == OrdersMode::Manager && waiting > 0)
        m_inbox->showMessage(waiting == 1 ? QString("1 edit request is waiting for your approval.")
                                          : QString("%1 edit requests are waiting for your approval.").arg(waiting),
                             Banner::Warning, 0);
    else if (m_mode == OrdersMode::Cashier && approved > 0)
        m_inbox->showMessage(approved == 1
                                 ? QString("1 order is approved for editing. Find it under Approved.")
                                 : QString("%1 orders are approved for editing. Find them under Approved.").arg(approved),
                             Banner::Info, 0);
    else
        m_inbox->clearMessage();

    applyFilter();
}

void OrdersPage::applyFilter()
{
    const QString keep = m_table->selectedKey();
    const QString needle = m_search->text().trimmed().toLower();
    const int filter = m_filter->currentIndex();

    m_table->setEmptyText(m_orders.empty() ? "There are no orders yet." : "No order matches.");
    m_table->clearRows();
    for (const pos::Order& order : m_orders) {
        const QString id = qs(order.id);
        const int access = m_access.value(id, -1);
        const bool waiting = access == int(pos::EditAccess::PendingApproval);
        const bool approved = access == int(pos::EditAccess::Allowed);

        if (filter == ShowOnHold && order.status != pos::OrderStatus::OnHold)
            continue;
        if (filter == ShowPaid && order.status != pos::OrderStatus::Paid)
            continue;
        if (filter == ShowApproved && !approved)
            continue;
        if (filter == ShowWaiting && !waiting)
            continue;

        const StatusInfo status = statusOf(order, access);
        if (!needle.isEmpty() && !id.contains(needle)
            && !qs(order.customerName).toLower().contains(needle)
            && !status.text.toLower().contains(needle))
            continue;

        const int row = m_table->addRow({ id, order.date.empty() ? QString("-") : qs(order.date),
                                          qs(order.customerName), status.text,
                                          isSettled(order) ? formatPkr(order.paymentAmount) : QString("-") },
                                        id);
        m_table->setTone(row, 3, status.tone);
    }
    m_table->selectKey(keep);                    // refreshes the details
}

const pos::Order* OrdersPage::selectedOrder() const
{
    const std::string id = ss(m_table->selectedKey());
    const auto it = std::find_if(m_orders.begin(), m_orders.end(),
                                 [&id](const pos::Order& o) { return o.id == id; });
    return (id.empty() || it == m_orders.end()) ? nullptr : &*it;
}

// ----------------------------------------------------------------- details

void OrdersPage::showSelected()
{
    const pos::Order* order = selectedOrder();
    m_placeholder->setVisible(order == nullptr);
    m_details->setVisible(order != nullptr);
    if (!order)
        return;

    const QString id = qs(order->id);
    const StatusInfo status = statusOf(*order, m_access.value(id, -1));

    m_heading->setText("Order " + id);
    m_status->setText(status.text);
    m_status->setProperty("kind", QString(status.kind));
    repolish(m_status);

    m_customer->setText(qs(order->customerName));
    m_contact->setText(order->customerContact.empty() ? QString("-") : qs(order->customerContact));
    m_when->setText(order->date.empty() ? QString("Not paid yet")
                                        : qs(order->date) + "   " + qs(order->time));
    QString editedAt;
    if (const auto approval = m_services.approvals().findByOrderId(order->id))
        editedAt = qs(pos::editedAtText(*approval));
    m_edited->setText(editedAt);
    m_edited->setVisible(!editedAt.isEmpty());
    m_editedCaption->setVisible(!editedAt.isEmpty());

    m_payment->setText(paymentText(*order));
    m_total->setText(isSettled(*order) ? formatPkr(order->paymentAmount) : QString("-"));

    QStringList lines;
    for (const pos::OrderItem& item : order->items) {
        if (lines.size() == kMaxItemLines) {
            lines << QString("and %1 more").arg(order->items.size() - kMaxItemLines);
            break;
        }
        const auto product = m_services.products().findById(item.productId);
        const QString name = product ? qs(product->name) : qs(item.productId);
        lines << (product ? formatQty(item.quantity, product->unit) : QString::number(item.quantity))
                     + " " + QString(kMultiply) + " " + name;
    }
    m_items->setText(lines.isEmpty() ? QString("-") : lines.join('\n'));

    configureActions(*order);
}

void OrdersPage::configureActions(const pos::Order& order)
{
    using pos::Action;
    const QString id = qs(order.id);

    m_onPrimary = nullptr;
    m_onSecond = nullptr;
    m_primaryBtn->hide();
    m_secondBtn->hide();
    QString note;

    auto primary = [this](const QString& text, std::function<void()> run) {
        m_primaryBtn->setText(text);
        m_primaryBtn->show();
        m_onPrimary = std::move(run);
    };
    auto second = [this](const QString& text, std::function<void()> run) {
        m_secondBtn->setText(text);
        m_secondBtn->show();
        m_onSecond = std::move(run);
    };

    switch (order.status) {
    case pos::OrderStatus::OnHold:
    case pos::OrderStatus::InProgress:
        if (can(int(Action::ResumeOrder)))
            primary("Resume order", [this, id]() { resume(id); });
        else
            note = "This order is on hold. A cashier can resume it.";
        break;

    case pos::OrderStatus::Archived:
        note = "Archived orders cannot be edited.";
        break;

    case pos::OrderStatus::Paid: {
        const auto access = static_cast<pos::EditAccess>(m_access.value(id, int(pos::EditAccess::NotFound)));
        switch (access) {
        case pos::EditAccess::Allowed:
            if (can(int(Action::EditOrder))) {
                primary("Edit order", [this, id]() { openForEdit(id); });
                note = "The manager approved an edit. It can be done once: after you save it, "
                       "this order cannot be edited again.";
            } else {
                note = "An edit is approved. A cashier can open this order now.";
            }
            break;
        case pos::EditAccess::NeedsApproval:
            if (can(int(Action::RequestApproval))) {
                primary("Ask for approval", [this, id]() { requestApproval(id); });
                note = QString("A paid order can be changed for %1 days after payment, once the manager approves.")
                           .arg(pos::kEditWindowDays);
            } else {
                note = "No edit has been requested for this order.";
            }
            break;
        case pos::EditAccess::PendingApproval:
            if (can(int(Action::ApproveEdit))) {
                primary("Approve edit", [this, id]() { decide(id, true); });
                second("Decline", [this, id]() { decide(id, false); });
                note = "A cashier asked to edit this order.";
            } else {
                note = "Waiting for the manager to approve the edit.";
            }
            break;
        case pos::EditAccess::Rejected:
            note = "The manager declined the edit.";
            break;
        case pos::EditAccess::Expired:
            note = QString("The %1-day editing window has passed.").arg(pos::kEditWindowDays);
            break;
        case pos::EditAccess::Archived:
            note = "Archived orders cannot be edited.";
            break;
        case pos::EditAccess::AlreadyEdited:
            note = "This order was already edited once. A paid order can only be edited one time.";
            break;
        case pos::EditAccess::NotFound:
            break;
        }

        // The manager can archive a paid order unless an edit request is waiting.
        if (can(int(Action::ApproveEdit)) && access != pos::EditAccess::PendingApproval)
            second("Archive order", [this, id]() { archive(id); });
        break;
    }
    }

    m_note->setText(note);
    m_note->setVisible(!note.isEmpty());

    const bool hasBill = isSettled(order) && can(int(Action::PrintBill))
                         && m_services.bills().findByOrderId(order.id).has_value();
    m_billBtn->setVisible(hasBill);
}

// ----------------------------------------------------------------- actions

void OrdersPage::resume(const QString& orderId)
{
    const PosActions::Result result = PosActions::resumeHeld(m_services, orderId);
    if (!result.ok) {
        m_banner->showMessage(result.message, Banner::Error, 4500);
        return;
    }
    emit openPageRequested(OrderPage::kTitle);
}

void OrdersPage::openForEdit(const QString& orderId)
{
    const PosActions::Result result = PosActions::openForEdit(m_services, orderId);
    if (!result.ok) {
        m_banner->showMessage(result.message, Banner::Error, 4500);
        return;
    }
    emit openPageRequested(OrderPage::kTitle);
}

void OrdersPage::requestApproval(const QString& orderId)
{
    if (!m_services.orderEdit().requestApproval(ss(orderId))) {
        m_banner->showMessage("The request could not be sent.", Banner::Error, 4000);
        return;
    }
    reload();
    m_banner->showMessage("Request sent. The manager can approve it under Orders.", Banner::Success, 4500);
}

void OrdersPage::decide(const QString& orderId, bool approve)
{
    const bool done = approve ? m_services.orderEdit().approve(ss(orderId))
                              : m_services.orderEdit().reject(ss(orderId));
    if (!done) {
        m_banner->showMessage("The decision could not be saved.", Banner::Error, 4000);
        return;
    }
    reload();
    m_banner->showMessage(approve ? "Edit approved. The cashier can open the order now."
                                  : "Edit declined.",
                          Banner::Success, 4000);
}

void OrdersPage::archive(const QString& orderId)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle("Archive order");
    box.setText(QString("Archive order %1?").arg(orderId));
    box.setInformativeText("An archived order can never be edited again.");
    QPushButton* confirm = box.addButton("Archive", QMessageBox::DestructiveRole);
    QPushButton* cancel = box.addButton("Cancel", QMessageBox::RejectRole);
    box.setDefaultButton(cancel);
    box.exec();
    if (box.clickedButton() != confirm)
        return;

    if (!m_services.orderEdit().archive(ss(orderId))) {
        m_banner->showMessage("The order could not be archived.", Banner::Error, 4000);
        return;
    }
    reload();
    m_banner->showMessage("Order archived.", Banner::Success, 3500);
}

void OrdersPage::viewBill(const QString& orderId)
{
    const std::optional<pos::Bill> bill = m_services.bills().findByOrderId(ss(orderId));
    if (!bill) {
        m_banner->showMessage("No bill was found for this order.", Banner::Error, 4000);
        return;
    }
    const pos::BillOrderInfo info = pos::billOrderInfoFor(m_services.orders(), bill->getOrderId(),
                                                               &m_services.approvals());
    BillDialog dialog(*bill, info, m_services.exporter(), this);
    dialog.exec();
}
