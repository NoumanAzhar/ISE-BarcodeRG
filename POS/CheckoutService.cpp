#include "CheckoutService.h"

#include <algorithm>
#include <exception>
#include <utility>
#include <vector>
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
                if (o.id.size() < 3) continue;
                try {
                    highest = max(highest, stoi(o.id.substr(0, o.id.size() - 2)));
                }
                catch (...) {
                    // ignore ids that are not numeric
                }
            }
            if (highest >= kMaxSequence) return false;
            out = highest + 1;
            return true;
        }

        // Returns the names of products whose stock could not be updated.
        vector<string> reduceStock(IProductRepository& products, const vector<OrderItem>& items) {
            vector<string> failed;
            for (const OrderItem& item : items) {
                auto product = products.findById(item.productId);
                if (!product) {
                    failed.push_back(item.productId);
                    continue;
                }
                product->availableQty = max(0.0, product->availableQty - item.quantity);
                if (!products.update(*product)) failed.push_back(product->name);
            }
            return failed;
        }

    } // namespace

    CheckoutService::CheckoutService(IOrderRepository& orders, IProductRepository& products,
        const IClock& clock, BillGenerator& billGenerator)
        : orders_(orders), products_(products), clock_(clock), bills_(billGenerator) {
    }

    CheckoutResult CheckoutService::checkout(Order order, PaymentMode mode, const string& merchant) {
        // 1. validate everything before anything is written
        if (order.items.empty())
            return fail(CheckoutStatus::EmptyOrder, "The order has no items.");
        if (mode == PaymentMode::None)
            return fail(CheckoutStatus::InvalidPayment, "Choose a payment mode.");
        if (needsMerchant(mode) && merchant.empty())
            return fail(CheckoutStatus::InvalidPayment, "Enter the merchant name for card or wallet payments.");

        double total = 0;
        for (const OrderItem& item : order.items) {
            if (!(item.quantity > kEpsilon))
                return fail(CheckoutStatus::InvalidQuantity, "Invalid quantity for product " + item.productId + ".");

            auto product = products_.findById(item.productId);   // fresh read: stock may have changed since the scan
            if (!product)
                return fail(CheckoutStatus::ProductNotFound, "Product not found: " + item.productId);
            if (item.quantity > product->availableQty + kEpsilon)
                return fail(CheckoutStatus::InsufficientStock, "Not enough stock for " + product->name + ".");

            total += (product->unitPrice + product->tax - product->discount) * item.quantity;
        }
        if (total < -kEpsilon)
            return fail(CheckoutStatus::InvalidTotal, "The discount is larger than the amount payable.");

        // 2. id, date and time
        int sequence = 0;
        if (!nextSequence(orders_, sequence))
            return fail(CheckoutStatus::OrderSaveFailed, "No order ids are left.");
        order.id = makeOrderId(sequence, clock_.dayOfMonth());
        order.date = clock_.date();
        order.time = clock_.time();

        // 3. mark paid (BillGenerator reads the status from the saved order)
        order.status = OrderStatus::Paid;
        order.paymentAmount = max(total, 0.0);
        order.paymentMode = mode;
        order.paymentMerchant = needsMerchant(mode) ? merchant : "";

        // 4. save order, then bill
        if (!orders_.save(order))
            return fail(CheckoutStatus::OrderSaveFailed, "Could not save the order.");

        CheckoutResult result;
        result.orderId = order.id;
        try {
            result.bill = bills_.generateBill(order.id);
        }
        catch (const exception& e) {
            result.status = CheckoutStatus::BillFailed;
            result.message = "Order " + order.id + " was saved as paid, but the bill failed: " + e.what();
            return result;
        }

        // 5. reduce stock
        vector<string> failed = reduceStock(products_, order.items);
        if (!failed.empty()) {
            string names;
            for (const string& name : failed) names += (names.empty() ? "" : ", ") + name;
            result.status = CheckoutStatus::StockUpdateFailed;
            result.message = "Bill generated, but stock was not updated for: " + names;
            return result;
        }

        result.message = "Order " + order.id + " paid. Bill " + result.bill->getBillId() + " generated.";
        return result;
    }

} // namespace pos
