#include "OrderTotals.h"
#include "Bill.h"
#include <utility>
#include <vector>

namespace pos {

    double lineTotal(const Product& product, double qty)
    {
        return (product.unitPrice + product.tax - product.discount) * qty;
    }

    OrderTotals computeTotals(const Order& order, const IProductRepository& products,
        double seasonalPercent, double specialPercent)
    {
        std::vector<std::pair<Product, double>> lines;
        for (const OrderItem& item : order.items) {
            const std::optional<Product> product = products.findById(item.productId);
            if (!product)
                continue;
            lines.push_back({ *product, item.quantity });
        }

        BillCalculator calculator;   // the same class the saved bill uses
        const BillTotals t = calculator.compute(lines, seasonalPercent, specialPercent);

        OrderTotals totals;
        totals.withoutTax = t.withoutTax;
        totals.tax = t.tax;
        totals.discount = t.productDiscount;
        totals.seasonalDiscount = t.seasonalDiscount;
        totals.specialDiscount = t.specialDiscount;
        totals.payable = t.payable;
        return totals;
    }

} // namespace pos
