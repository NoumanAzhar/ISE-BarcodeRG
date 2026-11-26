#pragma once

#include <optional>
#include <string>
#include <vector>

#include "order.h"

namespace pos {

    // ---- Layer 1: model ----
    // Used = the approved edit has been done (a paid order can be edited only once).
    enum class ApprovalStatus { Pending, Approved, Rejected, Used };

    struct Approval {
        std::string orderId, requestDate, requestTime;
        ApprovalStatus status = ApprovalStatus::Pending;
        std::string decisionDate, decisionTime;   // empty until the manager decides
        std::string editDate, editTime;           // empty until the approved edit is saved
    };

    // "2026-11-26  14:05:00" once the approved edit has been done; empty before that.
    std::string editedAtText(const Approval& approval);

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
    // orderId requestDate requestTime status decisionDate decisionTime editDate editTime
    // (older files with the first six fields still load)
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
    enum class EditAccess { Allowed, NeedsApproval, PendingApproval, Rejected, Expired, NotFound, AlreadyEdited };

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
        // Called once an edit of a paid order has been saved: it uses up the approval
        // and records the edit date and time. false if there was no approved edit.
        virtual bool markEdited(const std::string& orderId) = 0;
    };

    class OrderEditService : public IEditPolicy {
    public:
        OrderEditService(IOrderRepository& orders, IApprovalRepository& approvals, const IClock& clock);

        // on hold or in progress: Allowed;
        // paid: within kEditWindowDays of the order date (day 7 is still allowed) AND approved,
        // and only once: after the edit is saved the order is AlreadyEdited.
        EditCheck check(const std::string& orderId) const override;
        bool markEdited(const std::string& orderId) override;

        bool requestApproval(const std::string& orderId);   // cashier: paid order, window still open
        std::vector<Approval> pending() const;               // manager
        bool approve(const std::string& orderId);            // manager: needs a pending request
        bool reject(const std::string& orderId);             // manager: needs a pending request

    private:
        IOrderRepository& orders_;
        IApprovalRepository& approvals_;
        const IClock& clock_;
    };

} // namespace pos
