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
namespace pos {

namespace {
	constexpr size_t kBillFields = 14;
	constexpr size_t kDetailFields = 5;   // bill id, acronym, price, quantity, discount

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
	//BillDetail.txt: one row per product: billId acronym unitPrice quantity discount(total for the line)
	vector<Record> detailRecords(const Bill& b) {
		vector<Record> rows;
		for (const auto& item : b.getProducts()) {
			const Product& p = item.first;
			double qty = item.second;
			rows.push_back({ b.getBillId(), p.acronym, formatMoney(p.unitPrice),
							 formatQuantity(qty), formatMoney(p.discount * qty) });
		}
		return rows;
	}
	optional<Bill> recordToBill(const Record& r, const map<string, vector<Record>>& detailsById, const IProductRepository* catalog) {
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
		const vector<Record>& rows = found->second;

		vector<pair<pos::Product, double>> items;
		for (const Record& d : rows) {
			double price, qty, discount;
			if (d.size() != kDetailFields || !parseDouble(d[2], price) || !parseDouble(d[3], qty) ||
				!parseDouble(d[4], discount) || qty <= 0)
				return nullopt;
			Product p;
			p.acronym = d[1];
			p.unitPrice = price;
			p.discount = discount / qty;      // file stores per-unit discount x quantity
			if (catalog) {                    // name and tax are not stored in BillDetail.txt
				const optional<Product> known = catalog->findByAcronym(d[1]);
				if (known) { p.id = known->id; p.name = known->name; p.tax = known->tax; p.unit = known->unit; }
			}
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
FileBillRepository::FileBillRepository(string billFile, string detailFile, const IProductRepository* products)
	:billFile(billFile), detailFile(detailFile), products_(products) {}

bool FileBillRepository::save(const Bill& bill) {
	for (const Record& row : readRecords(billFile))
		if (!row.empty() && row[0] == bill.getBillId())	//need unique id
			return false;
	for (const Record& row : detailRecords(bill))
		if (!appendRecord(detailFile, row))
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
	for (const Record& row : detailRecords(bill)) details.push_back(row);
	return writeRecords(detailFile, details) && writeRecords(billFile, bills);
}
vector<Bill> FileBillRepository::all() const {
	warningList.clear();
	map<string, vector<Record>> detailsById;
	for (const Record& row : readRecords(detailFile))
		if (!row.empty())
			detailsById[row[0]].push_back(row);
	vector<Bill> result;
	const vector<Record> rows = readRecords(billFile);
	for (size_t i = 0; i < rows.size(); i++) {
		optional<Bill> bill = recordToBill(rows[i], detailsById, products_);
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
} // namespace pos
