#pragma once
#include<vector>
#include<string>
#include<utility>
#include"Product.h"
namespace pos {
struct BillTotals {
	double withoutTax = 0;
	double tax = 0;
	double productDiscount = 0;
	double seasonalDiscount = 0;
	double specialDiscount = 0;
	double payable = 0;
};
struct BillAdjustments {
	double seasonalPercent = 0;
	double specialPercent = 0;
	double amountReceived = 0;
	std::string date, time;
};
class BillCalculator {
public:
	BillTotals compute(const std::vector<std::pair<pos::Product, double>>& items, double seasonalPercent, double specialPercent) const;
};
class Bill {
	std::string billId, customerName, orderId;
	std::vector<std::pair<pos::Product,double>> products;	//storing each product and its quantity side by side
	std::string orderStatus;
	BillTotals totals;
	double amountReceived = 0;
	double balanceReturned = 0;
	std::string date, time;
public:
	Bill(const std::string& billId, const std::string& customerName, const std::string& orderId, const std::vector<std::pair<pos::Product, double>>& items, const std::string& status, const BillAdjustments& adj);
	Bill(const std::string& billId, const std::string& customerName, const std::string& orderId, const std::vector<std::pair<pos::Product, double>>& items, const std::string& status,
		 const BillTotals& totals, double amountReceived, double balanceReturned, const std::string& date, const std::string& time);

	//Getters
	const std::string& getBillId() const;
	const std::string& getCustomerName() const;
	const std::string& getOrderId() const;
	const std::vector<std::pair<pos::Product,double>>& getProducts() const;
	double getTotalWithoutTax() const;
	double getTotalTax() const;
	double getTotalDiscount() const;
	double getTotalBill() const;
	double getSeasonalDiscount() const;
	double getSpecialDiscount() const;
	double getAmountReceived() const;
	double getBalanceReturned() const;
	const std::string& getDate() const;
	const std::string& getTime() const;
	const std::string& getOrderStatus() const;
	//Setter: the bill status follows the order status (e.g. paid -> archived)
	void setOrderStatus(const std::string& status);
};
} // namespace pos
