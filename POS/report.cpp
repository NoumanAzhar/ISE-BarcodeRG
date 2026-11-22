#include "report.h"
#include <algorithm>
#include <map>
#include <utility>
#include "OrderTotals.h"
#include "bill_pdf.h"

namespace pos {

    namespace {

        bool isCompleted(const Order& order)
        {
            return order.status == OrderStatus::Paid || order.status == OrderStatus::Archived;
        }

        using BillsByOrder = std::map<std::string, Bill>;

        // One read of the bill file instead of one per order.
        BillsByOrder indexBills(const IBillRepository& bills)
        {
            BillsByOrder index;
            for (const Bill& bill : bills.all())
                index.emplace(bill.getOrderId(), bill);   // the first bill of an order wins
            return index;
        }

        // The bill total if the order has a bill, else what the order says it was paid.
        double amountOf(const Order& order, const BillsByOrder& bills)
        {
            const auto found = bills.find(order.id);
            return found != bills.end() ? found->second.getTotalBill() : order.paymentAmount;
        }

        // Product, seasonal and special discount together.
        double discountOf(const Order& order, const BillsByOrder& bills, const IProductRepository& products)
        {
            const auto found = bills.find(order.id);
            if (found == bills.end())
                return computeTotals(order, products).discount;
            const Bill& bill = found->second;
            return bill.getTotalDiscount() + bill.getSeasonalDiscount() + bill.getSpecialDiscount();
        }

        // A totals row with the same number of cells as the columns.
        std::vector<std::string> totalsRow(size_t columns, const std::string& label,
            const std::vector<std::pair<size_t, double>>& sums)
        {
            std::vector<std::string> row(columns, "");
            row[0] = label;
            for (const auto& sum : sums)
                row[sum.first] = toMoneyText(sum.second);
            return row;
        }

        std::string orderCountLabel(size_t count)
        {
            return "Total (" + std::to_string(count) + (count == 1 ? " order)" : " orders)");
        }

        std::string rangeText(const DateRange& range)
        {
            return range.from + " to " + range.to;
        }

        std::map<std::string, Product> indexProducts(const IProductRepository& products)
        {
            std::map<std::string, Product> index;
            for (const Product& product : products.all())
                index.emplace(product.id, product);
            return index;
        }

        bool lessByName(const Product& a, const Product& b)
        {
            return a.name != b.name ? a.name < b.name : a.id < b.id;
        }

    } // namespace

    ReportService::ReportService(const IOrderRepository& orders,
        const IBillRepository& bills,
        const IProductRepository& products)
        : orders_(orders), bills_(bills), products_(products)
    {
    }

    ReportTable ReportService::incompleteOrders() const
    {
        ReportTable table;
        table.title = "Incomplete orders";
        table.columns = { "Order Id", "Date", "Time", "Customer", "Status", "Items", "Amount" };

        double total = 0;
        size_t count = 0;
        for (const Order& order : orders_.all()) {
            if (isCompleted(order))
                continue;
            const double amount = computeTotals(order, products_).payable;
            table.rows.push_back({ order.id, order.date, order.time, order.customerName,
                                   toString(order.status), std::to_string(order.items.size()),
                                   toMoneyText(amount) });
            total += amount;
            ++count;
        }
        table.totals = totalsRow(table.columns.size(), orderCountLabel(count), { { 6, total } });
        return table;
    }

    ReportTable ReportService::completedOrders() const
    {
        ReportTable table;
        table.title = "Completed orders";
        table.columns = { "Order Id", "Date", "Time", "Customer", "Status", "Bill Id", "Amount" };

        const BillsByOrder bills = indexBills(bills_);
        double total = 0;
        size_t count = 0;
        for (const Order& order : orders_.all()) {
            if (!isCompleted(order))
                continue;
            const auto bill = bills.find(order.id);
            const double amount = amountOf(order, bills);
            table.rows.push_back({ order.id, order.date, order.time, order.customerName,
                                   toString(order.status),
                                   bill != bills.end() ? bill->second.getBillId() : "",
                                   toMoneyText(amount) });
            total += amount;
            ++count;
        }
        table.totals = totalsRow(table.columns.size(), orderCountLabel(count), { { 6, total } });
        return table;
    }

    ReportTable ReportService::cashSales(const DateRange& range) const
    {
        ReportTable table;
        table.title = "Cash sales " + rangeText(range);
        table.columns = { "Order Id", "Date", "Time", "Customer", "Amount" };

        const BillsByOrder bills = indexBills(bills_);
        double total = 0;
        size_t count = 0;
        for (const Order& order : orders_.all()) {
            if (!isCompleted(order) || order.paymentMode != PaymentMode::Cash || !inRange(order.date, range))
                continue;
            const double amount = amountOf(order, bills);
            table.rows.push_back({ order.id, order.date, order.time, order.customerName, toMoneyText(amount) });
            total += amount;
            ++count;
        }
        table.totals = totalsRow(table.columns.size(), orderCountLabel(count), { { 4, total } });
        return table;
    }

    ReportTable ReportService::allSales(const DateRange& range) const
    {
        ReportTable table;
        table.title = "All sales " + rangeText(range);
        table.columns = { "Order Id", "Date", "Time", "Customer", "Payment", "Discount", "Amount" };

        const BillsByOrder bills = indexBills(bills_);
        double totalDiscount = 0;
        double totalAmount = 0;
        size_t count = 0;
        for (const Order& order : orders_.all()) {
            if (!isCompleted(order) || !inRange(order.date, range))
                continue;
            const double discount = discountOf(order, bills, products_);
            const double amount = amountOf(order, bills);
            table.rows.push_back({ order.id, order.date, order.time, order.customerName,
                                   toString(order.paymentMode), toMoneyText(discount), toMoneyText(amount) });
            totalDiscount += discount;
            totalAmount += amount;
            ++count;
        }
        table.totals = totalsRow(table.columns.size(), orderCountLabel(count),
            { { 5, totalDiscount }, { 6, totalAmount } });
        return table;
    }

    ReportTable ReportService::topSold(int n) const
    {
        ReportTable table;
        table.title = "Top " + std::to_string(n < 0 ? 0 : n) + " most sold items";
        table.columns = { "Rank", "Acronym", "Name", "Sold", "Remaining" };
        if (n <= 0)
            return table;

        std::map<std::string, double> sold;   // product id -> quantity in completed orders
        for (const Order& order : orders_.all()) {
            if (!isCompleted(order))
                continue;
            for (const OrderItem& item : order.items)
                sold[item.productId] += item.quantity;
        }

        const std::map<std::string, Product> products = indexProducts(products_);
        std::vector<std::pair<Product, double>> ranked;   // products that still exist
        for (const auto& entry : sold) {
            const auto product = products.find(entry.first);
            if (product != products.end() && entry.second > 0)
                ranked.push_back({ product->second, entry.second });
        }
        std::stable_sort(ranked.begin(), ranked.end(),
            [](const std::pair<Product, double>& a, const std::pair<Product, double>& b) {
                return a.second != b.second ? a.second > b.second : lessByName(a.first, b.first);
            });

        for (size_t i = 0; i < ranked.size() && i < static_cast<size_t>(n); i++) {
            table.rows.push_back({ std::to_string(i + 1), ranked[i].first.acronym, ranked[i].first.name,
                                   toQuantityText(ranked[i].second),
                                   toQuantityText(ranked[i].first.availableQty) });
        }
        return table;
    }

    ReportTable ReportService::topRemaining(int n) const
    {
        ReportTable table;
        table.title = "Top " + std::to_string(n < 0 ? 0 : n) + " items with the most stock left";
        table.columns = { "Rank", "Acronym", "Name", "Remaining" };
        if (n <= 0)
            return table;

        std::vector<Product> products = products_.all();
        std::stable_sort(products.begin(), products.end(), [](const Product& a, const Product& b) {
            return a.availableQty != b.availableQty ? a.availableQty > b.availableQty : lessByName(a, b);
        });

        for (size_t i = 0; i < products.size() && i < static_cast<size_t>(n); i++) {
            table.rows.push_back({ std::to_string(i + 1), products[i].acronym, products[i].name,
                                   toQuantityText(products[i].availableQty) });
        }
        return table;
    }

} // namespace pos
