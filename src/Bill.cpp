#include<stdexcept>
#include"Bill.h"
using namespace std;

int Bill::counter = 100000000;		//baseId

Bill::Bill(const string& cName, const string& oId, const vector<pair<Product, int>>& items, const string& status) {
	customerName = cName;
	orderId = oId;
	products = items;
	if (products.empty()) {
		throw invalid_argument("Bill must have atleast 1 product");
	}
	totalWithoutTax = calcTotalWithoutTax();
	totalTax = calcTotalTax();
	totalDiscount = calcTotalDiscount();
	totalBill = calcTotalBill();
	orderStatus = status;
	billId = to_string(++counter);			
}

void Bill :: setCounter(int value) {
	if (value >= 100000000)
		counter = value;
}

//Calculations
double Bill::calcTotalWithoutTax() const {
	double total = 0;
	for (int i = 0; i < products.size(); i++) {
		total += products[i].first.getUnitPrice() * products[i].second;
	}
	return total;
}
double Bill::calcTotalTax() const {
	double total = 0;
	for (int i = 0; i < products.size(); i++) {
		total += products[i].first.getTax() * products[i].second;
	}
	return total;
}
double Bill::calcTotalDiscount() const {
	double total = 0;
	for (int i = 0; i < products.size(); i++) {
		total += products[i].first.getDiscount() * products[i].second;
	}
	return total;
}
double Bill::calcTotalBill() const {
	double totalWT = calcTotalWithoutTax();
	double tax = calcTotalTax();
	double disc = calcTotalDiscount();
	double total = totalWT + tax - disc;
	if (total < 0) {
		throw invalid_argument("Invalid Bill: Discount exceeds Amount");
	}
	else {
		return total;
	}
}

//Getters
const string& Bill::getBillId() const {
	return billId;
}
const string& Bill::getCustomerName() const {
	return customerName;
}
const string& Bill::getOrderId() const {
	return orderId;
}
const vector<pair<Product, int>>& Bill::getProducts() const {
	return products;
}
double Bill::getTotalWithoutTax() const {
	return totalWithoutTax;
}
double Bill::getTotalTax() const {
	return totalTax;
}
double Bill::getTotalDiscount() const {
	return totalDiscount;
}
double Bill::getTotalBill() const {
	return totalBill;
}
const string& Bill::getOrderStatus() const {
	return orderStatus;
}