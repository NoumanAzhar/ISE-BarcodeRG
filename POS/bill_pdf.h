#pragma once
#include <string>
#include "Bill.h"
#include "Data_Files.h"

namespace pos {

    class IOrderRepository;   // order.h

    // ---- Text helpers shared by the PDF and by the reports ----
    std::string toMoneyText(double value);      // 1250.5 -> "1250.50"
    std::string toQuantityText(double value);   // 2 -> "2", 2.5 -> "2.5"

    // The bill as monospace text (46 characters per line, lines joined by '\n').
    // This is also exactly what the PDF contains. The customer's phone number is
    // not part of Bill.txt (it is stored in Order.txt), so the caller passes it;
    // the Phone line is left out when it is empty.
    std::string formatBill(const Bill& bill, const std::string& customerContact = "");

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
        // With an order repository the PDF also shows the customer's phone number
        // (looked up by the bill's order id). Without one the Phone line is left out.
        explicit PdfBillExporter(const IOrderRepository* orders = nullptr);
        // Creates the folder if needed; writes a temp file, then renames it.
        bool exportBill(const Bill& bill, const std::string& path) override;
        // The PDF file content (for tests and for callers that want the bytes).
        std::string render(const Bill& bill) const;
    private:
        const IOrderRepository* orders_;
    };

} // namespace pos
