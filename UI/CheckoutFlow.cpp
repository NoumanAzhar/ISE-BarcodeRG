#include "CheckoutFlow.h"

#include "AppServices.h"
#include "BillDialog.h"
#include "CheckoutService.h"
#include "OrderTotals.h"
#include "PaymentDialog.h"

namespace {

void showBill(const pos::CheckoutResult& result, QWidget* parent)
{
    if (!result.bill)
        return;
    BillDialog dialog(*result.bill, parent);
    dialog.exec();
}

} // namespace

CheckoutFlow::CheckoutFlow(pos::AppServices& services)
    : m_services(services)
{
}

CheckoutFlow::Outcome CheckoutFlow::run(const QString& customerName,
                                        const QString& customerContact,
                                        QWidget* parent)
{
    // A copy: start() would empty the items, so the draft is reset only after saving.
    pos::Order order = m_services.order().current();
    order.customerName = customerName.toStdString();
    order.customerContact = customerContact.toStdString();

    const pos::OrderTotals totals = pos::computeTotals(order, m_services.products());

    PaymentDialog payment(totals.payable, parent);
    if (payment.exec() != QDialog::Accepted)
        return { Result::Cancelled, QString() };

    const pos::CheckoutResult result = m_services.checkout().checkout(
        order, payment.mode(), payment.merchant().toStdString());
    const QString message = QString::fromStdString(result.message);

    if (result.ok()) {
        showBill(result, parent);
        m_services.order().start("");
        return { Result::Saved, message };
    }

    // The order is already saved as paid; only the bill or the stock update failed.
    if (result.status == pos::CheckoutStatus::BillFailed
        || result.status == pos::CheckoutStatus::StockUpdateFailed) {
        showBill(result, parent);
        m_services.order().start("");
        return { Result::SavedWithWarning, message };
    }

    return { Result::Failed, message };
}
