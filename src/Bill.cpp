#include<stdexcept>
#include"Bill.h"
Bill::Bill(const string& cName, const string& oId, const vector<pair<Product, int>>& items, const string& status) {
	billId=to_string(++counter);
	customerName = cName;
	orderId = oId;
	products = items;
	totalWithoutTax = calcTotalWithoutTax();
	totalTax = calcTotalTax();
	totalDiscount = calcTotalDiscount();
	totalBill = calcTotalBill();
	orderStatus = status;
}

//Getters
string Bill::getBillId() const {
	return billId;
}
string Bill::getCustomerName() const {
	return customerName;
}
string Bill::getOrderId() const {
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
string Bill::getOrderStatus() const {
	return orderStatus;
}

//Calculations
double Bill::calcTotalWithoutTax() {
	double total=0;
	for (int i = 0; i < products.size(); i++) {
		total += products[i].first.getUnitPrice() * products[i].second;
	}
	return total;
}
double Bill::calcTotalTax() {
	double total = 0;
	for (int i = 0; i < products.size(); i++) {
		total += products[i].first.getTax() * products[i].second;
	}
	return total;
}
double Bill::calcTotalDiscount() {
	double total = 0;
	for (int i = 0; i < products.size(); i++) {
		total += products[i].first.getDiscount() * products[i].second;
	}
	return total;
}
double Bill::calcTotalBill() {
	double totalWT = calcTotalWithoutTax();
	double tax = calcTotalTax();
	double disc = calcTotalDiscount();
	if (disc > totalWT + tax) {
		throw invalid_argument("INVALID BILL");
	}
	else {
		return totalWT + tax - disc;
	}
}