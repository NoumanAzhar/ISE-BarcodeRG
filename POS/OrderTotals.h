#pragma once

#include "Product.h"
#include "order.h"

namespace pos {

    struct OrderTotals {
        double withoutTax = 0;
        double tax = 0;
        double discount = 0;           // product discounts
        double seasonalDiscount = 0;   // PKR amount
        double specialDiscount = 0;    // PKR amount
        double payable = 0;
    };

    // (unitPrice + tax - discount) * qty
    double lineTotal(const Product& product, double qty);

    // Adds up every item of the order. Items whose product is not found are skipped.
    // The maths is done by BillCalculator, so the live total matches the saved bill.
    // With no percents given you get the total before seasonal/special discount.
    OrderTotals computeTotals(const Order& order, const IProductRepository& products,
        double seasonalPercent = 0, double specialPercent = 0);

} // namespace pos
