#include "PosActions.h"

#include "AppServices.h"
#include "Format.h"
#include "Product.h"
#include "order.h"
#include "order_edit.h"

#include <QDate>

namespace PosActions {

const QString kWalkIn = QStringLiteral("Walk-in");

namespace {

bool draftHasItems(pos::AppServices& services)
{
    return !services.order().current().items.empty();
}

// Sets the current draft aside (on hold) so another order can be opened.
// Returns an empty text on success, otherwise the reason.
QString setDraftAside(pos::AppServices& services)
{
    if (!draftHasItems(services))
        return QString();
    if (isEditingPaid(services))
        return QStringLiteral("Finish or discard the order you are editing first.");
    if (!services.hold().hold())
        return QStringLiteral("The current order could not be put on hold.");
    return QString();
}

} // namespace

bool isEditingPaid(pos::AppServices& services)
{
    const pos::Order& order = services.order().current();
    return !order.id.empty() && order.status == pos::OrderStatus::Paid;
}

double seasonPercentToday(pos::AppServices& services)
{
    return services.seasons().percentOn(ss(QDate::currentDate().toString(Qt::ISODate)));
}

QString categoryName(pos::AppServices& services, const pos::Product& product)
{
    if (const auto category = services.categories().findById(product.categoryId))
        return qs(category->name);
    return QString();
}

Result holdDraft(pos::AppServices& services, const QString& name, const QString& contact)
{
    if (isEditingPaid(services))                   // holding would overwrite the saved paid order
        return { false, QStringLiteral("A paid order cannot be put on hold. Save or discard your changes."), QString() };

    const std::optional<std::string> id = services.hold().hold();
    if (!id)
        return { false, QStringLiteral("Add an item before putting an order on hold."), QString() };

    // The hold saves the order as Walk-in; keep what the cashier has typed so far.
    if (!name.isEmpty() || !contact.isEmpty()) {
        if (std::optional<pos::Order> saved = services.orders().findById(*id)) {
            if (!name.isEmpty())
                saved->customerName = ss(name);
            saved->customerContact = ss(contact);
            services.orders().save(*saved);
        }
    }

    services.order().start("");                  // a fresh draft for the next customer
    const QString orderId = qs(*id);
    return { true, QStringLiteral("Order %1 is on hold.").arg(orderId), orderId };
}

Result resumeHeld(pos::AppServices& services, const QString& orderId)
{
    const bool hadDraft = draftHasItems(services);
    if (const QString problem = setDraftAside(services); !problem.isEmpty())
        return { false, problem, QString() };

    if (!services.hold().resume(ss(orderId)))
        return { false, QStringLiteral("That order is no longer on hold."), QString() };

    QString message = QStringLiteral("Order %1 is open.").arg(orderId);
    if (hadDraft)
        message += QStringLiteral(" Your previous order is on hold.");
    return { true, message, orderId };
}

Result openForEdit(pos::AppServices& services, const QString& orderId)
{
    const std::optional<pos::Order> order = services.orders().findById(ss(orderId));
    if (!order)
        return { false, QStringLiteral("That order could not be found."), QString() };

    const pos::EditCheck check = services.orderEdit().check(ss(orderId));
    if (!check.allowed())
        return { false, qs(check.message), QString() };

    const bool hadDraft = draftHasItems(services);
    if (const QString problem = setDraftAside(services); !problem.isEmpty())
        return { false, problem, QString() };

    if (!services.order().load(*order))
        return { false, QStringLiteral("The order could not be opened."), QString() };

    QString message = QStringLiteral("Order %1 is open for editing.").arg(orderId);
    if (hadDraft)
        message += QStringLiteral(" Your previous order is on hold.");
    return { true, message, orderId };
}

} // namespace PosActions
