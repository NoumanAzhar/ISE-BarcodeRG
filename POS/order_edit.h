#pragma once

#include <string>

#include "BillStorage.h"
#include "order.h"

namespace pos {

    struct EditDecision {
        bool allowed;
        bool needsApproval;
        std::string reason;
    };

    class OrderEditPolicy {
    public:
        explicit OrderEditPolicy(const IClock& clock);

        // paidDate: date of the bill (yyyy-MM-dd), or empty if the order has no bill yet
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