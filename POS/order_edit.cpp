#include "order_edit.h"
#include "date_utils.h"

#include <string>
using namespace std;

namespace pos {

	OrderEditPolicy::OrderEditPolicy(const IClock& clock) : clock_(clock)
	{
	}

	EditDecision OrderEditPolicy::check(const Order& order, const string& paidDate) const
	{
		switch (order.status) {
		case OrderStatus::InProgress:
		case OrderStatus::OnHold:
			return { true, false, "" };
		case OrderStatus::Archived:
			return { false, false, "Archived orders cannot be edited." };
		case OrderStatus::Paid:
			break;
		}

		const string& since = paidDate.empty() ? order.date : paidDate;
		const string today = clock_.date();
		if (!isValidDate(since) || !isValidDate(today))
			return { false, false, "The payment date is not valid." };

		const int days = daysBetween(since, today);
		if (days < 0)
			return { false, false, "The payment date is not valid." };
		if (days > kEditWindowDays)
			return { false, false, "The edit window for a paid order is over." };
		return { true, true, "" };
	}

	OrderEditService::OrderEditService(const IOrderRepository& orders,
		const IBillRepository& bills, const OrderEditPolicy& policy)
		: orders_(orders), bills_(bills), policy_(policy)
	{
	}

	EditCheck OrderEditService::check(const string& orderId) const
	{
		auto order = orders_.findById(orderId);
		if (!order) return { false, false, "Order not found." };

		string paidDate;
		if (auto bill = bills_.findByOrderId(orderId)) paidDate = bill->getDate();

		const EditDecision decision = policy_.check(*order, paidDate);
		return { decision.allowed, decision.needsApproval, decision.reason };
	}

	OpenResult OrderEditService::open(const string& orderId, bool managerApproved) const
	{
		OpenResult result{ false, false, Order{}, "" };

		const EditCheck verdict = check(orderId);
		if (!verdict.allowed()) {
			result.error = verdict.message;
			return result;
		}
		if (verdict.needsApproval && !managerApproved) {
			result.needsApproval = true;
			result.error = "Manager approval is required.";
			return result;
		}

		auto order = orders_.findById(orderId);
		if (!order) {
			result.error = "Order not found.";
			return result;
		}
		result.ok = true;
		result.needsApproval = verdict.needsApproval;
		result.order = *order;
		return result;
	}

} // namespace pos