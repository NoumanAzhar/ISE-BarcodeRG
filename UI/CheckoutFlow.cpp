#include "CheckoutFlow.h"

#include "AppServices.h"
#include "BillDialog.h"
#include "bill_pdf.h"
#include "CheckoutService.h"
#include "OrderTotals.h"
#include "PaymentDialog.h"
#include "PosActions.h"

namespace {

void showBill(pos::AppServices& services, const pos::CheckoutResult& result, QWidget* parent)
{
    if (!result.bill)
        return;
    const pos::BillOrderInfo info = pos::billOrderInfoFor(services.orders(), result.bill->getOrderId());
    BillDialog dialog(*result.bill, info, services.exporter(), parent);
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

    // The dialog asks for the amount to pay after a special discount; the same
    // calculator that builds the saved bill answers it, so the two always agree.
    const double season = PosActions::seasonPercentToday(m_services);
    PaymentQuote quote;
    quote.seasonalPercent = season;
    quote.payableFor = [this, &order, season](double special) {
        return pos::computeTotals(order, m_services.products(), season, special).payable;
    };

    PaymentDialog dialog(quote, parent);
    if (dialog.exec() != QDialog::Accepted)
        return { Result::Cancelled, QString() };

    pos::Payment payment;
    payment.mode = dialog.mode();
    payment.merchant = dialog.merchant().toStdString();
    payment.amountReceived = dialog.amountReceived();
    payment.specialPercent = dialog.specialPercent();

    const pos::CheckoutResult result = m_services.checkout().checkout(order, payment);
    const QString message = QString::fromStdString(result.message);

    if (result.ok()) {
        showBill(m_services, result, parent);
        m_services.order().start("");
        return { Result::Saved, message };
    }

    // The order is already saved as paid; only the bill or the stock update failed.
    if (result.status == pos::CheckoutStatus::BillFailed
        || result.status == pos::CheckoutStatus::StockUpdateFailed) {
        showBill(m_services, result, parent);
        m_services.order().start("");
        return { Result::SavedWithWarning, message };
    }

    // Not saved (amount too low, special discount too high, edit no longer allowed, ...).
    return { Result::Failed, message };
}
