#pragma once
#include <optional>
#include <string>

#include "Bill.h"
#include "BillGenerator.h"
#include "Product.h"
#include "order.h"

namespace pos {

    enum class CheckoutStatus {
        Success,
        EmptyOrder,
        InvalidQuantity,
        InvalidPayment,
        ProductNotFound,
        InsufficientStock,
        InvalidTotal,        // discount larger than price + tax
        OrderSaveFailed,
        BillFailed,          // order IS saved (paid), bill could not be written
        StockUpdateFailed    // order and bill are saved, some stock was not reduced
    };

    struct CheckoutResult {
        CheckoutStatus status = CheckoutStatus::Success;
        std::string message;               // ready to show in the GUI
        std::string orderId;               // set once the order has an id
        std::optional<Bill> bill;          // set when the bill was generated

        bool ok() const { return status == CheckoutStatus::Success; }
    };

    // Runs "Generate Bill" as one operation so the GUI only makes one call:
    //   validate -> assign id/date/time -> mark paid -> save order -> bill -> reduce stock.
    // Reads the draft from OrderService::current(); it does not edit products
    // or bills itself, it only coordinates the repositories and BillGenerator.
    class CheckoutService {
    public:
        CheckoutService(IOrderRepository& orders,
            IProductRepository& products,
            const IClock& clock,
            BillGenerator& billGenerator);

        // merchant is required for Card and Wallet, ignored for Cash.
        CheckoutResult checkout(Order order, PaymentMode mode, const std::string& merchant);

    private:
        IOrderRepository& orders_;
        IProductRepository& products_;
        const IClock& clock_;
        BillGenerator& bills_;
    };

} // namespace pos
