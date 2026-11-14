#pragma once
#include<vector>
#include<string>
#include<utility>
#include"Product.h"
using namespace std;
class Bill {
	string billId, customerName, orderId;
	vector<pair<Product,int>> products;	//storing each product and its quantity side by side
	double totalWithoutTax, totalTax, totalDiscount, totalBill;
	string orderStatus;
	static int counter;
public:
	Bill(const string& cName,const string& oId,const vector<pair<Product,int>>& items,const string& status);

	//Getters
	string getBillId() const;
	string getCustomerName() const;
	string getOrderId() const;
	const vector<pair<Product,int>>& getProducts() const;
	double getTotalWithoutTax() const;
	double getTotalTax() const;
	double getTotalDiscount() const;
	double getTotalBill() const;
	string getOrderStatus() const;

	//Calculations
	double calcTotalWithoutTax();
	double calcTotalTax();
	double calcTotalDiscount();
	double calcTotalBill();

	void saveBill();
};
