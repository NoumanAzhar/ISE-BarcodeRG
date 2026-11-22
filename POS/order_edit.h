#pragma once

#include <climits>
#include <string>

#include "order.h"

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
}