#include "order_service.h"
#include <algorithm>
#include <cmath>
using namespace std;

namespace pos {
    namespace {

        constexpr double kEpsilon = 1e-9;

        bool fitsStock(double wanted, double available) { return wanted <= available + kEpsilon; }

        bool isWhole(double q) { return fabs(q - round(q)) < kEpsilon; }

        // > 0, finite , and whole for products sold by count
        bool validQuantity(const Product& p, double q) {
            if (!isfinite(q) || q <= kEpsilon) return false;
            return p.unit == MeasureUnit::Weight || isWhole(q);
        }

    } // namespace

    OrderService::OrderService(const IProductRepository& products) : products_(products) {
        start("");
    }

    void OrderService::start(const string& customerName, const string& contact) {
        order_ = Order{};
        order_.customerName = customerName;
        order_.customerContact = contact;
        order_.status = OrderStatus::InProgress;
        order_.paymentAmount = 0;
        order_.paymentMode = PaymentMode::None;
        // id, date and time stay empty: BillService::checkout assigns them
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

    AddResult OrderService::addByBarcode(const string& barcode, double quantity) {
        auto product = products_.findByBarcode(barcode);
        if (!product) return { AddStatus::UnknownBarcode, Product{}, 0 };

        const double already = quantityOf(product->id);
        if (!validQuantity(*product, quantity))
            return { AddStatus::InvalidQuantity, *product, already };

        const double total = already + quantity;
        if (!fitsStock(total, product->availableQty))
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
            !fitsStock(quantity, product->availableQty))
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

} // namespace pos