#pragma once
#include <string>
#include "Bill.h"
#include "Data_Files.h"
#include "order.h"
#include "order_edit.h"

namespace pos {

    // ---- Text helpers shared by the PDF and by the reports ----
    std::string toMoneyText(double value);      // 1250.5 -> "1250.50"
    std::string toQuantityText(double value);   // 2 -> "2", 2.5 -> "2.5"

    // What a bill prints that is not stored in Bill.txt: it lives in the order
    // (Order.txt). A field that is empty is left off the bill.
    struct BillOrderInfo {
        std::string customerContact;   // phone number, optional
        std::string paymentMode;       // see paymentModeText
        std::string editedAt;          // "date  time" of the approved edit; empty if never edited
    };

    // "Cash", "Bank-ABL", "Wallet-JazzCash". A card payment is shown as Bank and a
    // wallet payment as Wallet; the merchant name is added after a dash when the
    // order has one. Cash never shows a merchant. Empty for PaymentMode::None.
    std::string paymentModeText(PaymentMode mode, const std::string& merchant);

    // Phone number and payment mode taken from an order.
    BillOrderInfo billOrderInfo(const Order& order);

    // The same, looked up by order id. Empty info when the order is not found. With an
    // approval repository it also fills editedAt for an order that was edited.
    BillOrderInfo billOrderInfoFor(const IOrderRepository& orders, const std::string& orderId,
                                   const IApprovalRepository* approvals = nullptr);

    // The bill as monospace text (46 characters per line, lines joined by '\n').
    // This is also exactly what the PDF contains.
    std::string formatBill(const Bill& bill, const BillOrderInfo& info = BillOrderInfo());

    // kBillPdfDir + "/" + bill id + ".pdf"
    std::string billPdfPath(const Bill& bill);

    // ---- Layer 3: output interface ----
    // A Qt printer, an image or an e-mail exporter is a new class, not an edit here.
    class IBillExporter {
    public:
        virtual ~IBillExporter() = default;
        // false if the file could not be written
        virtual bool exportBill(const Bill& bill, const std::string& path) = 0;
    };

    // One-page PDF written by hand (no Qt, no library): Courier text, page height
    // grows with the number of lines. Characters outside plain ASCII print as '?'.
    class PdfBillExporter : public IBillExporter {
    public:
        // With an order repository the PDF also shows the customer's phone number and
        // the payment mode (looked up by the bill's order id). Without one those two
        // lines are left out.
        // With an approval repository it also shows the Edit Date of an edited order.
        explicit PdfBillExporter(const IOrderRepository* orders = nullptr,
                                 const IApprovalRepository* approvals = nullptr);
        // Creates the folder if needed; writes a temp file, then renames it.
        bool exportBill(const Bill& bill, const std::string& path) override;
        // The PDF file content (for tests and for callers that want the bytes).
        std::string render(const Bill& bill) const;
    private:
        const IOrderRepository* orders_;
        const IApprovalRepository* approvals_;
    };

} // namespace pos
