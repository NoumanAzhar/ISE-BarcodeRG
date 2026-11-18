#include<vector>
#include<stdexcept>
#include<utility>
#include"BillGenerator.h"
#include"BillStorage.h"
#include"FileUtils.h"
#include"Product.h"
#include"order.h"
#include"data_files.h"
using namespace std;

BillGenerator::BillGenerator(const pos::IOrderRepository& orders, const pos::IProductRepository& products)
	: orders_(orders), products_(products) {
}

void BillGenerator::loadBillCounter() {
	string id = FileUtils::readLastId(pos::kBillFile);
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

	optional<pos::Order> order = orders_.findById(orderId);
	if (!order)
		throw runtime_error("Order not found: " + orderId);
	vector<pair<pos::Product, double>> items;		//1 order can have multiple products

	for (size_t i = 0; i < order->items.size(); i++) {
		optional<pos::Product> p = products_.findById(order->items[i].productId);
		if (!p)
			throw runtime_error("Product not found: " + order->items[i].productId);
		items.push_back(make_pair(*p, order->items[i].quantity));
	}
	Bill bill(order->customerName, orderId, items, pos::toString(order->status));
	BillStorage::saveBill(bill);
	return bill;
}
