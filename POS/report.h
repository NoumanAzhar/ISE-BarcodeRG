#pragma once
#include <string>
#include <vector>
#include "BillStorage.h"
#include "Data_Files.h"
#include "Product.h"
#include "date_utils.h"
#include "order.h"

namespace pos {

    // A report as a table. Every cell is text (money with 2 decimals, quantities
    // without trailing zeros), so the GUI only has to show it.
    struct ReportTable {
        std::string title;
        std::vector<std::string> columns;
        std::vector<std::vector<std::string>> rows;   // each row has columns.size() cells
        std::vector<std::string> totals;              // columns.size() cells, or empty if the report has no total
    };

    // Read-only: collects data from the repositories, never writes.
    class ReportService {
    public:
        ReportService(const IOrderRepository& orders,
            const IBillRepository& bills,
            const IProductRepository& products);

        // Orders that are not paid or archived, with the total amount of all of them.
        ReportTable incompleteOrders() const;
        // Paid or archived orders, with the total amount of their bills.
        ReportTable completedOrders() const;
        // Completed orders paid in cash on a date inside the range, with the total.
        ReportTable cashSales(const DateRange& range) const;
        // Every completed order in the range with its discounts and amount, and both totals.
        ReportTable allSales(const DateRange& range) const;
        // Most sold products first (completed orders only), with the stock left.
        ReportTable topSold(int n = kTopN) const;
        // Products with the most stock left first, so they are not bought again.
        ReportTable topRemaining(int n = kTopN) const;

    private:
        const IOrderRepository& orders_;
        const IBillRepository& bills_;
        const IProductRepository& products_;
    };

} // namespace pos
