#include "OrderTotals.h"

namespace pos {

    double lineTotal(const Product& product, double qty)
    {
        return (product.unitPrice + product.tax - product.discount) * qty;
    }

    OrderTotals computeTotals(const Order& order, const IProductRepository& products)
    {
        OrderTotals totals;
        for (const OrderItem& item : order.items) {
            const std::optional<Product> product = products.findById(item.productId);
            if (!product)
                continue;
            totals.withoutTax += product->unitPrice * item.quantity;
            totals.tax += product->tax * item.quantity;
            totals.discount += product->discount * item.quantity;
        }

        totals.payable = totals.withoutTax + totals.tax - totals.discount;
        if (totals.payable < 0)
            totals.payable = 0;
        return totals;
    }

} // namespace pos
