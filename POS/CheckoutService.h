#pragma once
#include <optional>
#include <string>
#include "Bill.h"
#include "BillGenerator.h"
#include "Inventory.h"
#include "order.h"
#include "order_edit.h"
#include "season.h"

namespace pos {

    enum class CheckoutStatus {
        Success,
        EmptyOrder,
        InvalidPayment,
        InvalidQuantity,
        ProductNotFound,
        InsufficientStock,
        InvalidTotal,        // discount larger than price + tax
        OrderSaveFailed,
        BillFailed,          // order IS saved (paid), bill could not be written
        StockUpdateFailed,    // order and bill are saved, some stock was not reduced
        AmountTooLow,
        InvalidSpecialDiscount,
        EditNotAllowed
    };

    // The smallest banknote in Pakistan is PKR 10.
    inline constexpr double kSmallestNote = 10.0;

    // Cash to collect for a bill: the payable amount rounded UP to a whole multiple
    // of the smallest note (1234.50 -> 1240, 1240.00 -> 1240). The difference is
    // given back as change.
    double cashToCollect(double payable);

    struct Payment {
        PaymentMode mode = PaymentMode::None;
        std::string merchant;
        double amountReceived = 0;   // cash: 0 = collect cashToCollect(payable); card/wallet: ignored (exact)
        double specialPercent = 0;
    };

    struct CheckoutResult {
        CheckoutStatus status = CheckoutStatus::Success;
        std::string message;               // ready to show in the GUI
        std::string orderId;               // set once the order has an id
        std::optional<Bill> bill;          // set when the bill was generated

        bool ok() const { 
            return status == CheckoutStatus::Success; 
        }
    };

    // Runs "Generate Bill" as one operation so the GUI only makes one call:
    // No ID                ---> New Order
    // Id, Stored Not Paid  ---> Resumed on-hold order 
    // ID, Stored Paid      ---> Edited paid order 
    class CheckoutService {
    private:
        IOrderRepository& orders_;
        IProductRepository& products_;
        InventoryService& inventory_;
        const SeasonService& seasons_;
        IEditPolicy& editPolicy_;
        const IClock& clock_;
        BillGenerator& bills_;

    public:
        CheckoutService(IOrderRepository& orders,
            IProductRepository& products,
            InventoryService& inventory,
            const SeasonService& seasons,
            IEditPolicy& editPolicy,
            const IClock& clock,
            BillGenerator& billGenerator);

        CheckoutResult checkout(Order order,const Payment& payment);
    };

} // namespace pos
