#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "order.h"

namespace pos {

    // ---- Layer 1: model ----
    enum class ApprovalStatus { Pending, Approved, Rejected };

    struct Approval {
        std::string orderId, requestDate, requestTime;
        ApprovalStatus status = ApprovalStatus::Pending;
        std::string decisionDate, decisionTime;   // empty until the manager decides
    };

    std::string toString(ApprovalStatus status);
    std::optional<ApprovalStatus> parseApprovalStatus(const std::string& text);

    // ---- Layer 2: storage interface ----
    class IApprovalRepository {
    public:
        virtual ~IApprovalRepository() = default;
        virtual std::optional<Approval> findByOrderId(const std::string& orderId) const = 0;
        virtual std::vector<Approval> all() const = 0;
        virtual bool save(const Approval& a) = 0;   // upsert by order id
    };

    // ---- Layer 2: Approval.txt implementation ----
    // orderId requestDate requestTime status decisionDate decisionTime
    class FileApprovalRepository : public IApprovalRepository {
    public:
        explicit FileApprovalRepository(std::string path);
        std::optional<Approval> findByOrderId(const std::string& orderId) const override;
        std::vector<Approval> all() const override;
        bool save(const Approval& a) override;
    private:
        std::string path_;
    };

    // ---- Layer 3: rules ----
    enum class EditAccess { Allowed, NeedsApproval, PendingApproval, Rejected, Expired, Archived, NotFound };

    struct EditCheck {
        EditAccess access = EditAccess::NotFound;
        std::string message;
        bool allowed() const { return access == EditAccess::Allowed; }
    };

    // What CheckoutService asks before it saves an order that already exists.
    class IEditPolicy {
    public:
        virtual ~IEditPolicy() = default;
        virtual EditCheck check(const std::string& orderId) const = 0;
    };

    class OrderEditService : public IEditPolicy {
    public:
        OrderEditService(IOrderRepository& orders, IApprovalRepository& approvals, const IClock& clock);

        // on hold or in progress: Allowed; archived: never;
        // paid: within kEditWindowDays of the order date (day 7 is still allowed) AND approved.
        EditCheck check(const std::string& orderId) const override;

        bool requestApproval(const std::string& orderId);   // cashier: paid order, window still open
        std::vector<Approval> pending() const;               // manager
        bool approve(const std::string& orderId);            // manager: needs a pending request
        bool reject(const std::string& orderId);             // manager: needs a pending request
        bool archive(const std::string& orderId);            // manager: paid -> archived

        // Called after an order changes status here (archive), so other records that copy
        // the status (the bill) can follow without this class knowing about them.
        using StatusHandler = std::function<void(const Order&)>;
        void setStatusHandler(StatusHandler handler);

    private:
        IOrderRepository& orders_;
        IApprovalRepository& approvals_;
        const IClock& clock_;
        StatusHandler onStatus_;
    };

} // namespace pos
