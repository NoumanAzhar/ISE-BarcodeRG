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

    // What CheckoutService asks before it saves an order that already exists.
    struct EditCheck {
        bool permitted = false;
        bool needsApproval = false;
        std::string message;
        bool allowed() const { return permitted; }
    };

    class IEditPolicy {
    public:
        virtual ~IEditPolicy() = default;
        virtual EditCheck check(const std::string& orderId) const = 0;
    };

    struct OpenResult {
        bool ok;
        bool needsApproval;
        Order order;
        std::string error;
    };

    class OrderEditService : public IEditPolicy {
    public:
        OrderEditService(const IOrderRepository& orders, const IBillRepository& bills,
            const OrderEditPolicy& policy);

        // Looks the order and its bill up and applies OrderEditPolicy.
        EditCheck check(const std::string& orderId) const override;

        // Returns the order to edit, or the reason it was refused.
        OpenResult open(const std::string& orderId, bool managerApproved) const;

    private:
        const IOrderRepository& orders_;
        const IBillRepository& bills_;
        const OrderEditPolicy& policy_;
    };

} // namespace pos