#include "order_edit.h"
#include "Data_Files.h"
#include "date_utils.h"

#include <utility>
using namespace std;

namespace pos {

    namespace {

        const char* const kAlreadyEdited =
            "This order has already been edited once. A paid order can only be edited one time.";

        // true if a paid order can still be edited today (order date .. order date + 7 days)
        bool windowOpen(const Order& order, const IClock& clock, string& problem)
        {
            const string today = clock.date();
            if (!isValidDate(order.date) || !isValidDate(today)) {
                problem = "The payment date is not valid.";
                return false;
            }
            const int days = daysBetween(order.date, today);
            if (days < 0) {
                problem = "The payment date is not valid.";
                return false;
            }
            if (days > kEditWindowDays) {
                problem = "The edit window for a paid order is over.";
                return false;
            }
            return true;
        }

    } // namespace

    // ---------------- ApprovalStatus text ----------------

    string toString(ApprovalStatus status)
    {
        switch (status) {
        case ApprovalStatus::Pending:  return "pending";
        case ApprovalStatus::Approved: return "approved";
        case ApprovalStatus::Rejected: return "rejected";
        case ApprovalStatus::Used:     return "used";
        }
        return "pending";
    }

    optional<ApprovalStatus> parseApprovalStatus(const string& text)
    {
        if (text == "pending")  return ApprovalStatus::Pending;
        if (text == "approved") return ApprovalStatus::Approved;
        if (text == "rejected") return ApprovalStatus::Rejected;
        if (text == "used")     return ApprovalStatus::Used;
        return nullopt;
    }

    string editedAtText(const Approval& approval)
    {
        if (approval.status != ApprovalStatus::Used || approval.editDate.empty())
            return "";
        return approval.editTime.empty() ? approval.editDate : approval.editDate + "  " + approval.editTime;
    }

    // ---------------- FileApprovalRepository ----------------

    namespace {

        Record approvalToRecord(const Approval& a)
        {
            return { a.orderId, a.requestDate, a.requestTime, toString(a.status),
                     a.decisionDate, a.decisionTime, a.editDate, a.editTime };
        }

        optional<Approval> recordToApproval(const Record& r)
        {
            if ((r.size() != 6 && r.size() != 8) || r[0].empty())
                return nullopt;
            const optional<ApprovalStatus> status = parseApprovalStatus(r[3]);
            if (!status)
                return nullopt;
            Approval a;
            a.orderId = r[0];
            a.requestDate = r[1];
            a.requestTime = r[2];
            a.status = *status;
            a.decisionDate = r[4];
            a.decisionTime = r[5];
            if (r.size() == 8) {
                a.editDate = r[6];
                a.editTime = r[7];
            }
            return a;
        }

    } // namespace

    FileApprovalRepository::FileApprovalRepository(string path) : path_(move(path))
    {
    }

    vector<Approval> FileApprovalRepository::all() const
    {
        vector<Approval> result;
        for (const Record& row : readRecords(path_)) {
            const optional<Approval> a = recordToApproval(row);
            if (a)
                result.push_back(*a);
        }
        return result;
    }

    optional<Approval> FileApprovalRepository::findByOrderId(const string& orderId) const
    {
        for (const Approval& a : all())
            if (a.orderId == orderId)
                return a;
        return nullopt;
    }

    bool FileApprovalRepository::save(const Approval& a)
    {
        vector<Record> rows = readRecords(path_);
        for (Record& row : rows) {
            if (!row.empty() && row[0] == a.orderId) {
                row = approvalToRecord(a);
                return writeRecords(path_, rows);
            }
        }
        rows.push_back(approvalToRecord(a));
        return writeRecords(path_, rows);
    }

    // ---------------- OrderEditService ----------------

    OrderEditService::OrderEditService(IOrderRepository& orders, IApprovalRepository& approvals,
        const IClock& clock)
        : orders_(orders), approvals_(approvals), clock_(clock)
    {
    }

    EditCheck OrderEditService::check(const string& orderId) const
    {
        const optional<Order> order = orders_.findById(orderId);
        if (!order)
            return { EditAccess::NotFound, "Order not found." };

        switch (order->status) {
        case OrderStatus::InProgress:
        case OrderStatus::OnHold:
            return { EditAccess::Allowed, "" };
        case OrderStatus::Archived:
            return { EditAccess::Archived, "Archived orders cannot be edited." };
        case OrderStatus::Paid:
            break;
        }

        const optional<Approval> approval = approvals_.findByOrderId(orderId);
        // The approved edit can be used once; after that the order is closed for editing.
        if (approval && approval->status == ApprovalStatus::Used)
            return { EditAccess::AlreadyEdited, kAlreadyEdited };

        string problem;
        if (!windowOpen(*order, clock_, problem))
            return { EditAccess::Expired, problem };

        if (!approval)
            return { EditAccess::NeedsApproval, "Manager approval is required to edit a paid order." };
        switch (approval->status) {
        case ApprovalStatus::Approved:
            return { EditAccess::Allowed, "" };
        case ApprovalStatus::Pending:
            return { EditAccess::PendingApproval, "Waiting for the manager to approve this edit." };
        case ApprovalStatus::Used:
            return { EditAccess::AlreadyEdited, kAlreadyEdited };
        case ApprovalStatus::Rejected:
            break;
        }
        return { EditAccess::Rejected, "The manager rejected the edit of this order." };
    }

    bool OrderEditService::markEdited(const string& orderId)
    {
        optional<Approval> a = approvals_.findByOrderId(orderId);
        if (!a || a->status != ApprovalStatus::Approved)
            return false;
        a->status = ApprovalStatus::Used;
        a->editDate = clock_.date();
        a->editTime = clock_.time();
        return approvals_.save(*a);
    }

    bool OrderEditService::requestApproval(const string& orderId)
    {
        const optional<Order> order = orders_.findById(orderId);
        if (!order || order->status != OrderStatus::Paid)
            return false;
        string problem;
        if (!windowOpen(*order, clock_, problem))
            return false;

        const optional<Approval> existing = approvals_.findByOrderId(orderId);
        if (existing && existing->status != ApprovalStatus::Rejected)
            return false;                      // already pending or approved (a rejected one may be asked again)

        Approval a;
        a.orderId = orderId;
        a.requestDate = clock_.date();
        a.requestTime = clock_.time();
        a.status = ApprovalStatus::Pending;
        return approvals_.save(a);
    }

    vector<Approval> OrderEditService::pending() const
    {
        vector<Approval> result;
        for (const Approval& a : approvals_.all())
            if (a.status == ApprovalStatus::Pending)
                result.push_back(a);
        return result;
    }

    bool OrderEditService::approve(const string& orderId)
    {
        optional<Approval> a = approvals_.findByOrderId(orderId);
        if (!a || a->status != ApprovalStatus::Pending)
            return false;
        a->status = ApprovalStatus::Approved;
        a->decisionDate = clock_.date();
        a->decisionTime = clock_.time();
        return approvals_.save(*a);
    }

    bool OrderEditService::reject(const string& orderId)
    {
        optional<Approval> a = approvals_.findByOrderId(orderId);
        if (!a || a->status != ApprovalStatus::Pending)
            return false;
        a->status = ApprovalStatus::Rejected;
        a->decisionDate = clock_.date();
        a->decisionTime = clock_.time();
        return approvals_.save(*a);
    }

    bool OrderEditService::archive(const string& orderId)
    {
        optional<Order> order = orders_.findById(orderId);
        if (!order || order->status != OrderStatus::Paid)
            return false;
        order->status = OrderStatus::Archived;
        if (!orders_.save(*order))
            return false;
        if (onStatus_)
            onStatus_(*order);
        return true;
    }

    void OrderEditService::setStatusHandler(StatusHandler handler)
    {
        onStatus_ = std::move(handler);
    }

} // namespace pos
