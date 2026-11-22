#include<algorithm>
#include<cstdio>
#include<map>
#include<utility>
#include"BillStorage.h"
#include"Data_Files.h"
using namespace std;
using pos::Record;
using pos::readRecords;
using pos::writeRecords;
using pos::appendRecord;
using pos::Product;

namespace {
	constexpr size_t kBillFields = 14;
	constexpr size_t kDetailHeader = 2;   // bill id + product count
	constexpr size_t kItemFields = 4;     // acronym, price, quantity, discount

	//handling decimals
	string formatMoney(double v) { 
		char b[32]; snprintf(b, sizeof(b), "%.2f", v); 
		return b; 
	}
	string formatQuantity(double v) { 
		char b[32]; snprintf(b, sizeof(b), "%.10g", v); 
		return b; 
	}
	bool parseDouble(const string& text, double& out) {
		try { 
			size_t used = 0; 
			out = stod(text, &used); 
			return used == text.size(); 
		}
		catch (...) {
			return false; 
		}
	}
	Record billToRecord(const Bill& b) {
		return { b.getBillId(), b.getCustomerName(), b.getOrderId(),
				 formatMoney(b.getTotalWithoutTax()), formatMoney(b.getTotalTax()),
				 formatMoney(b.getTotalDiscount()), formatMoney(b.getTotalBill()),
				 formatMoney(b.getAmountReceived()), formatMoney(b.getBalanceReturned()),
				 b.getOrderStatus(), formatMoney(b.getSeasonalDiscount()),
				 formatMoney(b.getSpecialDiscount()), b.getDate(), b.getTime() };
	}
	//For BillDetails.txt
	Record detailRecord(const Bill& b) {
		const auto& items = b.getProducts();
		Record r = { b.getBillId(), to_string(items.size())};
		for (size_t i = 0; i < items.size(); i++) {
			const Product& p = items[i].first;
			double qty = items[i].second;
			r.push_back(p.acronym);
			r.push_back(formatMoney(p.unitPrice));
			r.push_back(formatQuantity(qty));
			r.push_back(formatMoney(p.discount * qty));
		}
		return r;
	}
	optional<Bill> recordToBill(const Record& r, const map<string, Record>& detailsById) {
		if (r.size() != kBillFields)
			return nullopt;
		BillTotals t;
		double received, balance;
		if (!parseDouble(r[3], t.withoutTax) || !parseDouble(r[4], t.tax) || !parseDouble(r[5], t.productDiscount)
			|| !parseDouble(r[6], t.payable) || !parseDouble(r[7], received) || !parseDouble(r[8], balance)
			|| !parseDouble(r[10], t.seasonalDiscount) || !parseDouble(r[11], t.specialDiscount))
			return nullopt;

		auto found = detailsById.find(r[0]);
		if (found == detailsById.end())
			return nullopt;
		const Record& d = found->second;

		double countValue;
		if (!parseDouble(d[1], countValue) || countValue < 1) 
			return nullopt;
		size_t n = static_cast<size_t>(countValue);
		if (d.size() != kDetailHeader + n * kItemFields) 
			return nullopt;

		vector<pair<pos::Product, double>> items;
		for (size_t i = 0; i < n; i++) {
			size_t at = kDetailHeader + i * kItemFields;
			double price, qty, discount;
			if (!parseDouble(d[at + 1], price) || !parseDouble(d[at + 2], qty) ||
				!parseDouble(d[at + 3], discount) || qty <= 0) 
				return nullopt;
			Product p;
			p.acronym = d[at];
			p.unitPrice = price;
			p.discount = discount / qty;      // file stores per-unit discount x quantity
			items.push_back({ p, qty });
		}
		try { 
			return Bill(r[0], r[1], r[2], items, r[9], t, received, balance, r[12], r[13]); 
		}
		catch (...) { 
			return nullopt;
		}
	}
}
FileBillRepository::FileBillRepository(string billFile, string detailFile) :billFile(billFile), detailFile(detailFile) {}

bool FileBillRepository::save(const Bill& bill) {
	for (const Record& row : readRecords(billFile))
		if (!row.empty() && row[0] == bill.getBillId())	//need unique id
			return false;
	if (!appendRecord(detailFile, detailRecord(bill)))
		return false;
	return appendRecord(billFile, billToRecord(bill));
}
bool FileBillRepository::replace(const Bill& bill) {
	vector<Record> bills = readRecords(billFile);
	bool found = false;
	for (Record& row : bills) {
		if (!row.empty() && row[0] == bill.getBillId()) {
			row = billToRecord(bill);
			found = true;
			break;
		}
	}
	if (!found) 
		return false;
	vector<Record> details;
	for (const Record& row : readRecords(detailFile))
		if (row.empty() || row[0] != bill.getBillId()) 
			details.push_back(row);
	details.push_back(detailRecord(bill));
	return writeRecords(detailFile, details) && writeRecords(billFile, bills);
}
vector<Bill> FileBillRepository::all() const {
	warningList.clear();
	map<string, Record> detailsById;
	for (const Record& row : readRecords(detailFile))
		if (row.size() >= kDetailHeader)
			detailsById[row[0]] = row;
	vector<Bill> result;
	const vector<Record> rows = readRecords(billFile);
	for (size_t i = 0; i < rows.size(); i++) {
		optional<Bill> bill = recordToBill(rows[i], detailsById);
		if (bill) 
			result.push_back(*bill);
		else 
			warningList.push_back("Bill.txt line " + to_string(i + 1) + " skipped (old layout or bad data)");
	}
	return result;
}
optional<Bill> FileBillRepository::findById(const string& billId) const {
	for (const Bill& b : all())
		if (b.getBillId() == billId) 
			return b;
	return nullopt;
}
optional<Bill> FileBillRepository::findByOrderId(const string& orderId) const {
	for (const Bill& b : all())
		if (b.getOrderId() == orderId) 
			return b;
	return nullopt;
}
string FileBillRepository::nextId() const {
	long long highest = 0;
	for (const Record& row : readRecords(billFile)) {
		if (row.size() != kBillFields)
			continue;					//damaged lines
		try { 
			highest = max(highest, stoll(row[0])); 
		}
		catch (...) {}
	}
	char buffer[16];
	//%09lld --> print a long long with 9 length (pad 0s on the left)
	snprintf(buffer, sizeof(buffer), "%09lld", highest + 1);
	return buffer;		//converts text into a string
}

const vector<string>& FileBillRepository::warnings() const {
	return warningList;
}