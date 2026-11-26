#pragma once
#include <optional>
#include <string>
#include <map>
#include <vector>
#include "order.h"
#include "Product.h"

namespace pos {

    enum class AddStatus { Added, UnknownBarcode, InvalidQuantity, InsufficientStock };

    struct AddResult {
        AddStatus status;
        Product product;          // empty Product if UnknownBarcode
        double quantityInOrder;   // total of this product in the order after the call
    };

    // Draft order being recorded by the cashier. Reads product data only;
    // saving the order is CheckoutService's job (SRP).
    class OrderService {
    public:
        explicit OrderService(const IProductRepository& products);

        void start(const std::string& customerName, const std::string& contact = "");
        AddResult addByBarcode(const std::string& barcode, double quantity = 1);
        bool setQuantity(const std::string& productId, double quantity);   // replaces the quantity
        bool removeItem(const std::string& productId);
        // True if removing this item would leave an already paid order (being edited) with no
        // items. A paid order must keep at least one item; removeItem() refuses it too.
        bool wouldEmptyPaidOrder(const std::string& productId) const;
        void clear();                                                 // items only; customer kept
        const Order& current() const;

        // Opens a saved order (on hold, in progress or paid) as the draft. Keeps its id.
        bool load(const Order& order);

    private:
        std::vector<OrderItem>::iterator findItem(const std::string& productId);
        double quantityOf(const std::string& productId) const;
        // stock already taken by the paid order being edited (returned to stock on checkout)
        double creditOf(const std::string& productId) const;

        const IProductRepository& products_;
        Order order_;
        std::map<std::string, double> paidQty_;   // empty unless a paid order is loaded
    };

    // Hold and resume. One job: move a draft to and from "on hold".
    class OrderHoldService {
    public:
        OrderHoldService(OrderService& draft, IOrderRepository& orders, const IClock& clock);

        // Saves the draft as OnHold and returns its id. No customer name needed
        // (stored as "Walk-in"). The id is assigned now and kept. No stock is reserved.
        // After a successful hold the draft is cleared so the cashier can scan the next customer.
        // nullopt if the draft has no items, is already paid, or the save fails.
        std::optional<std::string> hold();

        // An on-hold order becomes the draft. false if not found or not on hold.
        bool resume(const std::string& orderId);

        // On-hold orders, newest first.
        std::vector<Order> list() const;

    private:
        OrderService& draft_;
        IOrderRepository& orders_;
        const IClock& clock_;
    };

} // namespace pos
