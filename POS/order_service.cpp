#include "order_service.h"
#include <algorithm>
#include <cmath>
using namespace std;

namespace pos {
    namespace {

        constexpr double kEpsilon = 1e-9;
        constexpr int kMaxSequence = 999999;               // order id = 6-digit sequence + 2-digit day
        constexpr const char* kWalkInName = "Walk-in";

        bool fitsStock(double wanted, double available) { return wanted <= available + kEpsilon; }

        bool isWhole(double q) { return fabs(q - round(q)) < kEpsilon; }

        // > 0, finite , and whole for products sold by count
        bool validQuantity(const Product& p, double q) {
            if (!isfinite(q) || q <= kEpsilon) return false;
            return p.unit == MeasureUnit::Weight || isWhole(q);
        }

        // "00012314" -> 123 ; -1 if the id is not numeric
        int sequenceOf(const string& orderId) {
            if (orderId.size() < 3) return -1;
            try { return stoi(orderId.substr(0, orderId.size() - 2)); }
            catch (...) { return -1; }
        }

    } // namespace

    // ---------------- OrderService ----------------

    OrderService::OrderService(const IProductRepository& products) : products_(products) {
        start("");
    }

    void OrderService::start(const string& customerName, const string& contact) {
        order_ = Order{};
        paidQty_.clear();
        order_.customerName = customerName;
        order_.customerContact = contact;
        order_.status = OrderStatus::InProgress;
        order_.paymentAmount = 0;
        order_.paymentMode = PaymentMode::None;
        // id, date and time stay empty: CheckoutService (or hold) assigns them
    }

    vector<OrderItem>::iterator OrderService::findItem(const string& productId) {
        return find_if(order_.items.begin(), order_.items.end(),
            [&](const OrderItem& i) { return i.productId == productId; });
    }

    double OrderService::quantityOf(const string& productId) const {
        for (const OrderItem& i : order_.items)
            if (i.productId == productId) return i.quantity;
        return 0;
    }

    double OrderService::creditOf(const string& productId) const {
        auto it = paidQty_.find(productId);
        return it == paidQty_.end() ? 0.0 : it->second;
    }

    AddResult OrderService::addByBarcode(const string& barcode, double quantity) {
        ProductCatalog catalog(products_);
        auto product = catalog.lookup(barcode);
        if (!product) return { AddStatus::UnknownBarcode, Product{}, 0 };

        const double already = quantityOf(product->id);
        if (!validQuantity(*product, quantity))
            return { AddStatus::InvalidQuantity, *product, already };

        const double total = already + quantity;
        if (!fitsStock(total, product->availableQty + creditOf(product->id)))
            return { AddStatus::InsufficientStock, *product, already };

        auto it = findItem(product->id);
        if (it != order_.items.end()) it->quantity = total;               // scan again -> +quantity
        else order_.items.push_back({ product->id, product->barcode, quantity });
        return { AddStatus::Added, *product, total };
    }

    bool OrderService::setQuantity(const string& productId, double quantity) {
        auto it = findItem(productId);
        if (it == order_.items.end()) return false;
        auto product = products_.findById(productId);
        if (!product || !validQuantity(*product, quantity) ||
            !fitsStock(quantity, product->availableQty + creditOf(productId)))
            return false;                                                 // order left unchanged
        it->quantity = quantity;
        return true;
    }

    bool OrderService::removeItem(const string& productId) {
        auto it = findItem(productId);
        if (it == order_.items.end()) return false;
        order_.items.erase(it);
        return true;
    }

    void OrderService::clear() { order_.items.clear(); }

    const Order& OrderService::current() const { return order_; }

    bool OrderService::load(const Order& order) {
        if (order.status == OrderStatus::Archived) return false;
        order_ = order;
        paidQty_.clear();
        if (order.status == OrderStatus::Paid)
            for (const OrderItem& i : order.items) paidQty_[i.productId] += i.quantity;
        return true;
    }

    // ---------------- OrderHoldService ----------------

    OrderHoldService::OrderHoldService(OrderService& draft, IOrderRepository& orders, const IClock& clock)
        : draft_(draft), orders_(orders), clock_(clock) {
    }

    optional<string> OrderHoldService::hold() {
        Order order = draft_.current();
        if (order.items.empty()) return nullopt;
        if (order.status == OrderStatus::Paid || order.status == OrderStatus::Archived) return nullopt;

        if (order.id.empty()) {
            int highest = 0;
            for (const Order& o : orders_.all())
                highest = max(highest, sequenceOf(o.id));
            if (highest >= kMaxSequence) return nullopt;
            order.id = makeOrderId(highest + 1, clock_.dayOfMonth());
        }
        if (order.customerName.empty()) order.customerName = kWalkInName;
        order.status = OrderStatus::OnHold;
        // date and time stay as they are (empty): they are set when the order is paid

        if (!orders_.save(order)) return nullopt;
        draft_.start("");
        return order.id;
    }

    bool OrderHoldService::resume(const string& orderId) {
        auto order = orders_.findById(orderId);
        if (!order || order->status != OrderStatus::OnHold) return false;
        return draft_.load(*order);
    }

    vector<Order> OrderHoldService::list() const {
        vector<Order> held;
        for (const Order& o : orders_.all())
            if (o.status == OrderStatus::OnHold) held.push_back(o);
        sort(held.begin(), held.end(), [](const Order& a, const Order& b) {
            return sequenceOf(a.id) > sequenceOf(b.id);
            });
        return held;
    }

} // namespace pos
