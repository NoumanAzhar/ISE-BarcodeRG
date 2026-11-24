#include "order.h"
#include "Data_Files.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <map>
#include <utility>
using namespace std;

namespace pos {

	string toString(OrderStatus status) {
		switch (status) {
		case OrderStatus::InProgress:
			return "in_progress";
		case OrderStatus::OnHold:
			return "on_hold";
		case OrderStatus::Paid:
			return "paid";
		case OrderStatus::Archived:
			return "archived";
		}
		return "in_progress";
	}

	optional<OrderStatus> parseOrderStatus(const string& text) {
		if (text == "in_progress")
			return OrderStatus::InProgress;
		else if (text == "on_hold")
			return OrderStatus::OnHold;
		else if (text == "paid")
			return OrderStatus::Paid;
		else if (text == "archived")
			return OrderStatus::Archived;
		else
			return nullopt;
	}

	string toString(PaymentMode mode) {
		switch (mode) {
		case PaymentMode::None:
			return "none";
		case PaymentMode::Cash:
			return "cash";
		case PaymentMode::Card:
			return "card";
		case PaymentMode::Wallet:
			return "wallet";
		}
		return "none";
	}

	optional<PaymentMode> parsePaymentMode(const string& text) {
		if (text == "none")
			return PaymentMode::None;
		else if (text == "cash")
			return PaymentMode::Cash;
		else if (text == "card")
			return PaymentMode::Card;
		else if (text == "wallet")
			return PaymentMode::Wallet;
		else
			return nullopt;
	}

	string makeOrderId(int sequence, int dayOfMonth)
	{
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%06d%02d", sequence, dayOfMonth);
		return buffer;
	}

	namespace {
		tm localNow()
		{
			time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
			tm result{};
#ifdef _WIN32
			localtime_s(&result, &now);
#else
			localtime_r(&now, &result);
#endif
			return result;
		}

		string formatNow(const char* pattern)
		{
			tm t = localNow();
			char buffer[32];
			strftime(buffer, sizeof(buffer), pattern, &t);
			return buffer;
		}
	}

	string SystemClock::date() const
	{
		return formatNow("%Y-%m-%d");
	}

	string SystemClock::time() const
	{
		return formatNow("%H:%M:%S");
	}

	int SystemClock::dayOfMonth() const
	{
		return localNow().tm_mday;
	}

	namespace {

		string formatMoney(double value)
		{
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%.2f", value);
			return buffer;
		}

		string formatQuantity(double value)
		{
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%.10g", value);   // 2 -> "2", 2.5 -> "2.5"
			return buffer;
		}

		bool parseDouble(const string& text, double& out)
		{
			try {
				size_t used = 0;
				out = stod(text, &used);
				return used == text.size();
			}
			catch (...) {
				return false;
			}
		}

		Record orderToRecord(const Order& o)
		{
			return { o.id, o.date, o.time, o.customerName, o.customerContact,
					 toString(o.status), formatMoney(o.paymentAmount),
					 toString(o.paymentMode), o.paymentMerchant };
		}

		optional<Order> recordToOrder(const Record& r)
		{
			if (r.size() < 9) return nullopt;
			auto status = parseOrderStatus(r[5]);
			auto mode = parsePaymentMode(r[7]);
			double amount = 0;
			if (!status || !mode || !parseDouble(r[6], amount)) return nullopt;

			Order o;
			o.id = r[0];
			o.date = r[1];
			o.time = r[2];
			o.customerName = r[3];
			o.customerContact = r[4];
			o.status = *status;
			o.paymentAmount = amount;
			o.paymentMode = *mode;
			o.paymentMerchant = r[8];
			return o;
		}

		// One OrderDetail.txt row: the order id, then productId barcode quantity
		// for every item, e.g. 00012314 1001 123456 2 1002 654321 1
		Record itemsToRecord(const Order& order)
		{
			Record row;
			row.push_back(order.id);
			for (const OrderItem& item : order.items) {
				row.push_back(item.productId);
				row.push_back(item.barcode);
				row.push_back(formatQuantity(item.quantity));
			}
			return row;
		}

		// Reverse of itemsToRecord. Returns no items if the row is malformed.
		vector<OrderItem> recordToItems(const Record& r)
		{
			vector<OrderItem> items;
			if (r.size() < 4 || (r.size() - 1) % 3 != 0) return items;

			for (size_t i = 1; i < r.size(); i += 3) {
				double quantity = 0;
				if (!parseDouble(r[i + 2], quantity)) return {};
				items.push_back(OrderItem{ r[i], r[i + 1], quantity });
			}
			return items;
		}

	}

	FileOrderRepository::FileOrderRepository(string orderFile, string detailFile)
		: orderFile_(move(orderFile)), detailFile_(move(detailFile))
	{
	}

	bool FileOrderRepository::save(const Order& order)
	{
		// Order.txt: replace this order's line, or add a new one
		vector<Record> orders = readRecords(orderFile_);
		bool replaced = false;
		for (Record& row : orders) {
			if (!row.empty() && row[0] == order.id) {
				row = orderToRecord(order);
				replaced = true;
				break;
			}
		}
		if (!replaced) orders.push_back(orderToRecord(order));

		// OrderDetail.txt: drop the old row of this order, add one row with all items
		vector<Record> details;
		for (const Record& row : readRecords(detailFile_)) {
			if (row.empty() || row[0] != order.id) details.push_back(row);
		}
		if (!order.items.empty()) details.push_back(itemsToRecord(order));

		// details first, so an order line never exists without its items
		return writeRecords(detailFile_, details) && writeRecords(orderFile_, orders);
	}

	vector<Order> FileOrderRepository::all() const
	{
		map<string, vector<OrderItem>> itemsByOrder;
		for (const Record& row : readRecords(detailFile_)) {
			if (!row.empty()) itemsByOrder[row[0]] = recordToItems(row);
		}

		vector<Order> result;
		for (const Record& row : readRecords(orderFile_)) {
			auto order = recordToOrder(row);
			if (!order) continue;                       // skip malformed lines
			auto found = itemsByOrder.find(order->id);
			if (found != itemsByOrder.end()) order->items = found->second;
			result.push_back(*order);
		}
		return result;
	}

	optional<Order> FileOrderRepository::findById(const string& id) const
	{
		for (const Order& order : all()) {
			if (order.id == id) return order;
		}
		return nullopt;
	}

	bool FileOrderRepository::containsProduct(const string& productId) const
	{
		for (const Record& row : readRecords(detailFile_)) {
			// row: orderId, then productId barcode quantity for every item
			for (size_t i = 1; i < row.size(); i += 3) {
				if (row[i] == productId) return true;
			}
		}
		return false;
	}

	int FileOrderRepository::count() const
	{
		return static_cast<int>(readRecords(orderFile_).size());
	}

}