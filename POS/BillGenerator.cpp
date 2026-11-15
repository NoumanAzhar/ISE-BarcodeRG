#include<vector>
#include"BillGenerator.h"
#include"BillStorage.h"
#include"FileUtils.h"
#include"Product.h"
#include"Order.h"
using namespace std;
void BillGenerator::loadBillCounter() {
	string id=FileUtils::readLastId("Bill.txt");
	if (id.empty())
		return;
	try {
		Bill::setCounter(stoi(id));
	}
	catch (...) {
		//leave the counter as it is
	}
}
Bill BillGenerator::generateBill(const string& orderId) {
	loadBillCounter();

	Order order = OrderStorage::findOrderById(orderId);		//Might have to make changes here
	string customerName = order.getCustomerName();
	string status = order.getStatus();
	const vector<Order>& prods = order.getProducts();				//1 order can have multiple products
	vector<pair<Product, int>> items;

	for (int i = 0; i < prods.size(); i++) {
		Product p = ProductStorage::findById(prods[i].getProductId());
		items.push_back(make_pair(p, prods[i].getQuantity()));
	}
	Bill bill(customerName, orderId, items, status);
	BillStorage::saveBill(bill);
	return bill;

}