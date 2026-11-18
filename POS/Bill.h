#pragma once
#include<vector>
#include<string>
#include<utility>
#include"Product.h"
class Bill {
	std::string billId, customerName, orderId;
	std::vector<std::pair<pos::Product,double>> products;	//storing each product and its quantity side by side
	double totalWithoutTax, totalTax, totalDiscount, totalBill;
	std::string orderStatus;
	static int counter;

	//Calculations
	double calcTotalWithoutTax() const;
	double calcTotalTax() const;
	double calcTotalDiscount() const;
	double calcTotalBill() const;
public:
	Bill(const std::string& cName,const std::string& oId,const std::vector<std::pair<pos::Product,double>>& items,const std::string& status);
	static void setCounter(int);

	//Getters
	const std::string& getBillId() const;
	const std::string& getCustomerName() const;
	const std::string& getOrderId() const;
	const std::vector<std::pair<pos::Product,double>>& getProducts() const;
	double getTotalWithoutTax() const;
	double getTotalTax() const;
	double getTotalDiscount() const;
	double getTotalBill() const;
	const std::string& getOrderStatus() const;
};
