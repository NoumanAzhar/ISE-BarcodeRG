#include "bill_pdf.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>
#include <vector>
#include "order.h"

namespace pos {

    namespace {

        constexpr size_t kLineWidth = 46;   // characters per line
        constexpr int kFontSize = 9;
        constexpr int kLeading = 12;        // distance between two baselines
        constexpr int kMargin = 36;
        constexpr int kPageWidth = 322;     // 46 Courier characters (248 pt) + two margins
        constexpr int kMinPageHeight = 200;

        struct Line {
            std::string text;
            bool bold = false;
        };

        // ---- layout helpers ----
        std::string rule()
        {
            return std::string(kLineWidth, '-');
        }

        std::string cut(const std::string& text, size_t width)
        {
            return text.size() <= width ? text : text.substr(0, width);
        }

        std::string padRight(const std::string& text, size_t width)
        {
            const std::string t = cut(text, width);
            return t + std::string(width - t.size(), ' ');
        }

        // Numbers are never cut: a misaligned number is better than a wrong one.
        std::string padLeft(const std::string& text, size_t width)
        {
            if (text.size() >= width)
                return text;
            return std::string(width - text.size(), ' ') + text;
        }

        std::string centered(const std::string& text)
        {
            const std::string t = cut(text, kLineWidth);
            return std::string((kLineWidth - t.size()) / 2, ' ') + t;
        }

        // label at the left edge, value at the right edge
        std::string labelValue(const std::string& label, const std::string& value)
        {
            const std::string l = cut(label, kLineWidth - value.size() - 1);
            return l + std::string(kLineWidth - l.size() - value.size(), ' ') + value;
        }

        constexpr size_t kLabelWidth = 12;   // "Payment Mode" is the longest label

        std::string field(const std::string& label, const std::string& value)
        {
            return padRight(label, kLabelWidth) + ": " +
                cut(value.empty() ? "-" : value, kLineWidth - kLabelWidth - 2);
        }

        std::string upper(std::string text)
        {
            for (char& c : text) {
                if (c >= 'a' && c <= 'z')
                    c = static_cast<char>(c - 'a' + 'A');
            }
            return text;
        }

        std::string minus(double value)
        {
            return value > 0 ? "-" + toMoneyText(value) : toMoneyText(value);
        }

        // Every line of the bill; formatBill and the PDF both use this.
        std::vector<Line> billLines(const Bill& bill, const BillOrderInfo& info)
        {
            std::vector<Line> lines;
            auto add = [&lines](const std::string& text, bool bold = false) {
                lines.push_back({ text, bold });
            };

            add(centered("BARCODERG"), true);
            add(centered("Bill"));
            add(rule());
            add(field("Bill id", bill.getBillId()));
            add(field("Order id", bill.getOrderId()));
            add(field("Customer", bill.getCustomerName()));
            if (!info.customerContact.empty())
                add(field("Phone", info.customerContact));
            if (!info.paymentMode.empty())
                add(field("Payment Mode", info.paymentMode));
            add(field("Date", bill.getDate().empty() ? "" : bill.getDate() + "  " + bill.getTime()));
            add(field("Status", upper(bill.getOrderStatus())));
            add(rule());

            add(padRight("ITEM", 8) + " " + padLeft("QTY", 7) + " " + padLeft("PRICE", 9) + " " +
                padLeft("DISC", 8) + " " + padLeft("AMOUNT", 10), true);
            for (const auto& entry : bill.getProducts()) {
                const double quantity = entry.second;
                const double amount = entry.first.unitPrice * quantity;
                const double discount = entry.first.discount * quantity;
                add(padRight(entry.first.acronym, 8) + " " + padLeft(toQuantityText(quantity), 7) + " " +
                    padLeft(toMoneyText(entry.first.unitPrice), 9) + " " +
                    padLeft(toMoneyText(discount), 8) + " " + padLeft(toMoneyText(amount), 10));
            }
            add(rule());

            add(labelValue("Subtotal", toMoneyText(bill.getTotalWithoutTax())));
            add(labelValue("Tax", toMoneyText(bill.getTotalTax())));
            add(labelValue("Product discount", minus(bill.getTotalDiscount())));
            if (bill.getSeasonalDiscount() > 0)
                add(labelValue("Seasonal discount", minus(bill.getSeasonalDiscount())));
            if (bill.getSpecialDiscount() > 0)
                add(labelValue("Special discount", minus(bill.getSpecialDiscount())));
            add(rule());
            add(labelValue("PAYABLE", "PKR " + toMoneyText(bill.getTotalBill())), true);
            add(labelValue("Amount received", toMoneyText(bill.getAmountReceived())));
            add(labelValue("Balance returned", toMoneyText(bill.getBalanceReturned())));
            add(rule());
            add(centered("Thank you"));
            return lines;
        }

        // PDF string: escape \ ( ), anything outside printable ASCII becomes '?'
        std::string pdfText(const std::string& text)
        {
            std::string out;
            for (char c : text) {
                const unsigned char u = static_cast<unsigned char>(c);
                if (u < 32 || u > 126)
                    out += '?';
                else if (c == '\\' || c == '(' || c == ')')
                    out += std::string("\\") + c;
                else
                    out += c;
            }
            return out;
        }

    } // namespace

    // ---------------- text helpers ----------------

    std::string toMoneyText(double value)
    {
        char buffer[48];
        std::snprintf(buffer, sizeof(buffer), "%.2f", value);
        return buffer;
    }

    std::string toQuantityText(double value)
    {
        char buffer[48];
        std::snprintf(buffer, sizeof(buffer), "%.10g", value);   // 2 -> "2", 2.5 -> "2.5"
        return buffer;
    }

    std::string paymentModeText(PaymentMode mode, const std::string& merchant)
    {
        const std::string suffix = merchant.empty() ? "" : "-" + merchant;
        switch (mode) {
        case PaymentMode::Cash:   return "Cash";
        case PaymentMode::Card:   return "Bank" + suffix;
        case PaymentMode::Wallet: return "Wallet" + suffix;
        case PaymentMode::None:   break;
        }
        return "";
    }

    BillOrderInfo billOrderInfo(const Order& order)
    {
        return { order.customerContact, paymentModeText(order.paymentMode, order.paymentMerchant) };
    }

    BillOrderInfo billOrderInfoFor(const IOrderRepository& orders, const std::string& orderId)
    {
        const std::optional<Order> order = orders.findById(orderId);
        return order ? billOrderInfo(*order) : BillOrderInfo();
    }

    std::string formatBill(const Bill& bill, const BillOrderInfo& info)
    {
        std::string text;
        const std::vector<Line> lines = billLines(bill, info);
        for (size_t i = 0; i < lines.size(); i++) {
            if (i > 0)
                text += '\n';
            text += lines[i].text;
        }
        return text;
    }

    std::string billPdfPath(const Bill& bill)
    {
        return std::string(kBillPdfDir) + "/" + bill.getBillId() + ".pdf";
    }

    // ---------------- PdfBillExporter ----------------

    PdfBillExporter::PdfBillExporter(const IOrderRepository* orders)
        : orders_(orders)
    {
    }

    std::string PdfBillExporter::render(const Bill& bill) const
    {
        BillOrderInfo info;
        if (orders_)
            info = billOrderInfoFor(*orders_, bill.getOrderId());
        const std::vector<Line> lines = billLines(bill, info);

        int height = 2 * kMargin + static_cast<int>(lines.size()) * kLeading;
        if (height < kMinPageHeight)
            height = kMinPageHeight;

        // Page content: first baseline near the top, then one line per T*.
        std::string content = "BT\n" + std::to_string(kLeading) + " TL\n" +
            std::to_string(kMargin) + " " + std::to_string(height - kMargin - kFontSize) + " Td\n";
        bool bold = false;
        bool fontSet = false;
        for (const Line& line : lines) {
            if (!fontSet || line.bold != bold) {
                bold = line.bold;
                fontSet = true;
                content += std::string(bold ? "/F2 " : "/F1 ") + std::to_string(kFontSize) + " Tf\n";
            }
            content += "(" + pdfText(line.text) + ") Tj\nT*\n";
        }
        content += "ET";

        // Objects 1..6; their byte offsets go into the cross-reference table.
        std::string pdf = "%PDF-1.4\n";
        std::vector<size_t> offsets(7, 0);
        auto addObject = [&pdf, &offsets](int number, const std::string& body) {
            offsets[number] = pdf.size();
            pdf += std::to_string(number) + " 0 obj\n" + body + "\nendobj\n";
        };
        addObject(1, "<< /Type /Catalog /Pages 2 0 R >>");
        addObject(2, "<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
        addObject(3, "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + std::to_string(kPageWidth) + " " +
            std::to_string(height) + "] /Contents 4 0 R "
            "/Resources << /Font << /F1 5 0 R /F2 6 0 R >> >> >>");
        addObject(4, "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content + "\nendstream");
        addObject(5, "<< /Type /Font /Subtype /Type1 /BaseFont /Courier >>");
        addObject(6, "<< /Type /Font /Subtype /Type1 /BaseFont /Courier-Bold >>");

        const size_t xrefAt = pdf.size();
        pdf += "xref\n0 7\n0000000000 65535 f \n";
        for (int number = 1; number <= 6; number++) {
            char entry[32];
            std::snprintf(entry, sizeof(entry), "%010zu 00000 n \n", offsets[number]);   // exactly 20 bytes
            pdf += entry;
        }
        pdf += "trailer\n<< /Size 7 /Root 1 0 R >>\nstartxref\n" + std::to_string(xrefAt) + "\n%%EOF\n";
        return pdf;
    }

    bool PdfBillExporter::exportBill(const Bill& bill, const std::string& path)
    {
        namespace fs = std::filesystem;
        std::error_code ignored;
        const fs::path target(path);
        if (target.has_parent_path())
            fs::create_directories(target.parent_path(), ignored);

        const std::string temp = path + ".tmp";
        {
            std::ofstream out(temp, std::ios::binary);
            if (!out.is_open())
                return false;
            const std::string pdf = render(bill);
            out.write(pdf.data(), static_cast<std::streamsize>(pdf.size()));
            out.close();
            if (out.fail()) {
                fs::remove(temp, ignored);
                return false;
            }
        }

        std::error_code renamed;
        fs::rename(temp, target, renamed);   // replaces an existing file
        if (renamed) {
            fs::remove(temp, ignored);
            return false;
        }
        return true;
    }

} // namespace pos
