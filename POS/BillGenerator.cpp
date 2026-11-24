#include<optional>
#include<vector>
#include<stdexcept>
#include<utility>
#include"BillGenerator.h"
#include"Product.h"
#include"order.h"
using namespace std;
namespace pos {

BillGenerator::BillGenerator(const pos::IOrderRepository& orders, const pos::IProductRepository& products, IBillRepository& bills)
	: orders_(orders), products_(products), bills_(bills) {}

Bill BillGenerator::generateBill(const string& orderId, const BillAdjustments& adj) {
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
	optional<Bill> existing = bills_.findByOrderId(orderId);
	string id = existing ? existing->getBillId() : bills_.nextId();		//if doesn't exist assign new id

	Bill bill(id, order->customerName, orderId, items, pos::toString(order->status), adj);
	bool saved = existing ? bills_.replace(bill) : bills_.save(bill);	//either replace the old bill or make a new if id doesn't exist
	if(!saved)
		throw runtime_error("Could not save the bill for order " + orderId);
	return bill;
}
} // namespace pos
