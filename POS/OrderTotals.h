#pragma once

#include "Product.h"
#include "order.h"

namespace pos {

    struct OrderTotals {
        double withoutTax = 0;
        double tax = 0;
        double discount = 0;
        double payable = 0;
    };

    // (unitPrice + tax - discount) * qty
    double lineTotal(const Product& product, double qty);

    // Adds up every item of the order. Items whose product is not found are skipped.
    // payable = withoutTax + tax - discount, never below 0.
    OrderTotals computeTotals(const Order& order, const IProductRepository& products);

} // namespace pos
