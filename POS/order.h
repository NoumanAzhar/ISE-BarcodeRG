#pragma once

#include <optional>
#include <string>
#include <vector>

namespace pos {

    enum class OrderStatus { InProgress, OnHold, Paid };
    enum class PaymentMode { None, Cash, Card, Wallet };

    struct OrderItem {
        std::string productId;
        std::string barcode;
        double quantity;
    };

    struct Order {
        std::string id;                // 8 digits, last two = day of month
        std::string date, time;        // yyyy-MM-dd, HH:mm:ss
        std::string customerName, customerContact;   // contact optional
        OrderStatus status;
        double paymentAmount;
        PaymentMode paymentMode;
        std::string paymentMerchant;   // optional (e.g. "ABL", "JazzCash")
        std::vector<OrderItem> items;
    };

    std::string toString(OrderStatus status);
    std::optional<OrderStatus> parseOrderStatus(const std::string& text);
    std::string toString(PaymentMode mode);
    std::optional<PaymentMode> parsePaymentMode(const std::string& text);

    class IClock {
    public:
        virtual ~IClock() = default;
        virtual std::string date() const = 0;
        virtual std::string time() const = 0;
        virtual int dayOfMonth() const = 0;
    };

    class SystemClock : public IClock {
    public:
        std::string date() const override;
        std::string time() const override;
        int dayOfMonth() const override;
    };

    // (123, 14) -> "00012314"
    std::string makeOrderId(int sequence, int dayOfMonth);

    class IOrderRepository {
    public:
        virtual ~IOrderRepository() = default;
        virtual bool save(const Order& order) = 0;   // Order.txt line + OrderDetail.txt rows
        virtual std::optional<Order> findById(const std::string& id) const = 0;
        virtual std::vector<Order> all() const = 0;
        virtual int count() const = 0;
        // true if the product appears in any saved order (it has been sold)
        virtual bool containsProduct(const std::string& productId) const = 0;
    };

    class FileOrderRepository : public IOrderRepository {
    public:
        FileOrderRepository(std::string orderFile, std::string detailFile);
        bool save(const Order& order) override;
        std::optional<Order> findById(const std::string& id) const override;
        std::vector<Order> all() const override;
        int count() const override;
        bool containsProduct(const std::string& productId) const override;

    private:
        std::string orderFile_;
        std::string detailFile_;
    };

} // namespace pos