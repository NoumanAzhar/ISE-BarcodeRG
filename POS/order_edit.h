#pragma once

#include <climits>
#include <string>

#include "order.h"
#include "bill_service.h"

namespace pos {
	inline constexpr int kInvalidDays = INT_MAX;

	int daysBetween(const std::string& fromDate, const std::string& toDate);

	struct EditDecision {
		bool allowed;
		bool needsApproval;
		std::string reason;
	};

	class OrderEditPolicy {
	public:
		explicit OrderEditPolicy(const IClock& clock);
		
		EditDecision check(const Order& order, const std::string& paidDate) const;

	private:
		const IClock& clock_;
	};
	
	struct OpenResult {
		bool ok;
		bool needsApproval;
		Order order;
		std::string error;
	};

	class OrderEditService {
	public:
		OrderEditService(const IOrderRepository& orders, const IBillRepository& bills,
			const OrderEditPolicy& policy);

		// Returns the order to edit, or the reason it was refused.
		OpenResult open(const std::string& orderId, bool managerApproved) const;

	private:
		const IOrderRepository& orders_;
		const IBillRepository& bills_;
		const OrderEditPolicy& policy_;
	};
}