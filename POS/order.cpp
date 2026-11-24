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
			return "in-progress";
		case OrderStatus::OnHold:
			return "on-hold";
		case OrderStatus::Paid:
			return "paid";
		case OrderStatus::Archived:
			return "archived";
		}
		return "in-progress";
	}

	optional<OrderStatus> parseOrderStatus(const string& text) {
		if (text == "in-progress")
			return OrderStatus::InProgress;
		else if (text == "on-hold")
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

		// OrderDetail.txt: one row per item: orderId productId barcode quantity
			vector<Record> itemsToRecords(const Order& order)
			{
				vector<Record> rows;
				for (const OrderItem& item : order.items)
					rows.push_back({ order.id, item.productId, item.barcode, formatQuantity(item.quantity) });
				return rows;
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
		for (const Record& row : itemsToRecords(order)) details.push_back(row);

		// details first, so an order line never exists without its items
		return writeRecords(detailFile_, details) && writeRecords(orderFile_, orders);
	}

	vector<Order> FileOrderRepository::all() const
	{
		map<string, vector<OrderItem>> itemsByOrder;
		for (const Record& row : readRecords(detailFile_)) {
			double quantity = 0;
				if (row.size() != 4 || !parseDouble(row[3], quantity)) continue;
				itemsByOrder[row[0]].push_back(OrderItem{ row[1], row[2], quantity });
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

	int FileOrderRepository::count() const
	{
		return static_cast<int>(readRecords(orderFile_).size());
	}

	bool FileOrderRepository::containsProduct(const string& productId) const
	{
		for (const Order& order : all())
			for (const OrderItem& item : order.items)
				if (item.productId == productId) return true;
		return false;
	}

}
