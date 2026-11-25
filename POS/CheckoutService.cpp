#include "CheckoutService.h"
#include <algorithm>
#include <cmath>
#include <exception>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "Data_Files.h"
using namespace std;

namespace pos {
    namespace {

        constexpr double kEpsilon = 1e-9;
        constexpr int kMaxSequence = 999999;   // order id = 6-digit sequence + 2-digit day

        bool needsMerchant(PaymentMode mode) {
            return mode == PaymentMode::Card || mode == PaymentMode::Wallet;
        }

        CheckoutResult fail(CheckoutStatus status, string message) {
            CheckoutResult result;
            result.status = status;
            result.message = move(message);
            return result;
        }

        // Next free order sequence = highest sequence on file + 1 (not count()+1,
        // so deleting a line can never make a new order reuse an old id).
        bool nextSequence(const IOrderRepository& orders, int& out) {
            int highest = 0;
            for (const Order& o : orders.all()) {
                if (o.id.size() < 3) 
                    continue;
                try {
                    highest = max(highest, stoi(o.id.substr(0, o.id.size() - 2)));
                }
                catch (...) {
                    // ignore ids that are not numeric
                }
            }
            if (highest >= kMaxSequence) 
                return false;
            out = highest + 1;
            return true;
        }

        //product id->total quantity
        map<string, double> quantitiesOf(const vector<OrderItem>& items) {
            map<string, double> result;
            for (const OrderItem& item : items)
                result[item.productId] += item.quantity;
            return result;
        }

    } // namespace

    double cashToCollect(double payable) {
        if (!(payable > kEpsilon))
            return 0.0;
        return ceil(payable / kSmallestNote - kEpsilon) * kSmallestNote;
    }

    CheckoutService::CheckoutService(IOrderRepository& orders, IProductRepository& products,
        InventoryService& inventory, const SeasonService& seasons,
        const IEditPolicy& editPolicy, const IClock& clock,
        BillGenerator& billGenerator)
        : orders_(orders), products_(products), inventory_(inventory), seasons_(seasons), editPolicy_(editPolicy), clock_(clock), bills_(billGenerator) {}

    CheckoutResult CheckoutService::checkout(Order order, const Payment& payment) {

        // 1. validate everything before anything is written
        if (order.items.empty())
            return fail(CheckoutStatus::EmptyOrder, "The order has no items.");
        if (payment.mode == PaymentMode::None)
            return fail(CheckoutStatus::InvalidPayment, "Choose a payment mode.");
        if (needsMerchant(payment.mode) && payment.merchant.empty())
            return fail(CheckoutStatus::InvalidPayment, "Enter the merchant name for card or wallet payments.");
        if (!(payment.specialPercent >= 0) || payment.specialPercent > kMaxSpecialPercent + kEpsilon)
            return fail(CheckoutStatus::InvalidSpecialDiscount, "The special discount must be between 0 and 8 percent.");
        if (!isfinite(payment.amountReceived) || payment.amountReceived < 0)
            return fail(CheckoutStatus::AmountTooLow, "Invalid amount received.");

        //2. Path: new/resumed on-hold/edited paid order
        optional<Order> stored;
        bool isEdit = false;
        if (!order.id.empty()) {
            stored = orders_.findById(order.id);
            if (!stored)
                return fail(CheckoutStatus::EditNotAllowed, "Order " + order.id + " was not found.");
            EditCheck check = editPolicy_.check(order.id);
            if (!check.allowed())
                return fail(CheckoutStatus::EditNotAllowed, check.message);
            isEdit = (stored->status == OrderStatus::Paid);
        }
        
        //Stock already taken by the old paid order(only edit has any)
        map<string, double>oldQty;
        if (isEdit)
            oldQty = quantitiesOf(stored->items);
        const map<string, double>newQty = quantitiesOf(order.items);

        //3. check every line against fresh product data
        vector<pair<Product, double>> lines;
        for (const OrderItem& item : order.items) {
            if (!(item.quantity > kEpsilon))
                return fail(CheckoutStatus::InvalidQuantity, "Invalid quantity for product " + item.productId + ".");
        }
        for (const auto& entry : newQty) {
            auto product = products_.findById(entry.first);   // fresh read: stock may have changed since the scan
            if (!product)
                return fail(CheckoutStatus::ProductNotFound, "Product not found: " + entry.first);
            auto old = oldQty.find(entry.first);
            const double available = product->availableQty + (old == oldQty.end() ? 0.0 : old->second);
            if (entry.second > available + kEpsilon)
                return fail(CheckoutStatus::InsufficientStock, "Not enough stock for " + product->name + ".");
            lines.push_back({ *product, entry.second });
        }

        //4. id, date and time
        if (isEdit) {
            order.id = stored->id;
            order.date = stored->date;
            order.time = stored->time;
        }
        else {
            if (order.id.empty()) {
                int sequence = 0;
                if (!nextSequence(orders_, sequence))
                    return fail(CheckoutStatus::OrderSaveFailed, "No order ids are left.");
                order.id = makeOrderId(sequence, clock_.dayOfMonth());
            }
            order.date = clock_.date();     // date and time are set when the order is paid
            order.time = clock_.time();
        }

        //5. totals 
        const double seasonalPercent = seasons_.percentOn(order.date);
        BillCalculator calculator;
        const BillTotals totals = calculator.compute(lines, seasonalPercent, payment.specialPercent);
        if(totals.withoutTax+totals.tax-totals.productDiscount< -kEpsilon)
            return fail(CheckoutStatus::InvalidTotal, "The discount is larger than the amount payable.");
        // Cash is taken in whole notes: with no amount given, the cashier collects the
        // payable rounded up to the next multiple of 10 and the rest is change.
        double amountReceived = payment.amountReceived;
        if (payment.mode == PaymentMode::Cash && amountReceived <= 0)
            amountReceived = cashToCollect(totals.payable);
        if (amountReceived > 0 && amountReceived < totals.payable - kEpsilon)
            return fail(CheckoutStatus::AmountTooLow, "The amount received is less than the amount payable.");

        // 6. mark paid
        order.status = OrderStatus::Paid;
        order.paymentAmount = totals.payable;
        order.paymentMode = payment.mode;
        order.paymentMerchant = needsMerchant(payment.mode) ? payment.merchant : "";

        // 7. stock first (old - new, a new order has old = 0): if anything fails here
        //    the changes already made are undone and nothing has been saved yet.
        set<string> ids;
        for (const auto& e : oldQty)
            ids.insert(e.first);
        for (const auto& e : newQty)
            ids.insert(e.first);

        vector<pair<string, double>> applied;
        auto undoStock = [&]() {
            for (auto it = applied.rbegin(); it != applied.rend(); ++it)
                inventory_.adjust(it->first, -it->second);
        };
        for (const string& id : ids) {
            auto o = oldQty.find(id);
            auto n = newQty.find(id);
            const double delta = (o == oldQty.end() ? 0.0 : o->second) - (n == newQty.end() ? 0.0 : n->second);
            if (fabs(delta) < kEpsilon)
                continue;
            if (!inventory_.adjust(id, delta)) {
                undoStock();
                return fail(CheckoutStatus::StockUpdateFailed, "Stock could not be updated for product " + id + ". Nothing was saved.");
            }
            applied.push_back({ id, delta });
        }

        // 8. save the order (new, or replaced under the same id)
        if (!orders_.save(order)) {
            undoStock();
            return fail(CheckoutStatus::OrderSaveFailed, "Could not save the order.");
        }

        // 9. bill (new, or replaced under the same id)
        CheckoutResult result;
        result.orderId = order.id;

        BillAdjustments adj;
        adj.seasonalPercent = seasonalPercent;
        adj.specialPercent = payment.specialPercent;
        adj.amountReceived = amountReceived;
        adj.date = order.date;
        adj.time = order.time;
        try {
            result.bill = bills_.generateBill(order.id, adj);
        }
        catch (const exception& e) {
            result.status = CheckoutStatus::BillFailed;
            result.message = "Order " + order.id + " was saved as paid and stock was updated, but the bill failed: " + e.what();
            return result;
        }
        result.message = string("Order ") + order.id + (isEdit ? " updated. Bill " : " paid. Bill ")
            + result.bill->getBillId() + (isEdit ? " replaced." : " generated.");
        return result;
    }

} // namespace pos