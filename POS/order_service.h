#pragma once
#include <string>
#include "order.h"
#include "product.h"
using namespace std;

namespace pos {

    enum class AddStatus { Added, UnknownBarcode, InvalidQuantity, InsufficientStock };

    struct AddResult {
        AddStatus status;
        Product product;          // empty Product if UnknownBarcode
        double quantityInOrder;   // total of this product in the order after the call
    };

    // Draft order being recorded by the cashier. Reads product data only;
    // saving the order is BillService::checkout's job (SRP).
    class OrderService {
    public:
        explicit OrderService(const IProductRepository& products);

        void start(const string& customerName, const string& contact = "");
        AddResult addByBarcode(const string& barcode, double quantity = 1);
        bool setQuantity(const string& productId, double quantity);   // replaces the quantity
        bool removeItem(const string& productId);
        void clear();                                                 // items only; customer kept
        const Order& current() const;

    private:
        vector<OrderItem>::iterator findItem(const string& productId);
        double quantityOf(const string& productId) const;

        const IProductRepository& products_;
        Order order_;
    };

} // namespace pos