#include<stdexcept>
#include<cmath>
#include"Bill.h"
using namespace std;
namespace pos {
//we need to round some amounts to 2 decimal places
namespace 
{
	double roundTo2(double value)
	{
		return round(value * 100) / 100;
	}
}
BillTotals BillCalculator::compute(const vector<pair<pos::Product, double>>& items,double seasonalPercent,double specialPercent) const
{
	BillTotals t;
	for (int i = 0; i < items.size(); i++)
	{
		const pos::Product& p = items[i].first;
		double quantity = items[i].second;
		t.withoutTax += p.unitPrice * quantity;
		t.tax += p.tax * quantity;
		t.productDiscount += p.discount * quantity;
	}
	//base amount
	double base = t.withoutTax + t.tax - t.productDiscount;
	t.seasonalDiscount = base * (seasonalPercent / 100);
	t.specialDiscount = base * (specialPercent / 100);
	t.payable = base - t.seasonalDiscount - t.specialDiscount;
	if (t.payable < 0)
		t.payable = 0;
	t.withoutTax = roundTo2(t.withoutTax);
	t.tax = roundTo2(t.tax);
	t.productDiscount = roundTo2(t.productDiscount);
	t.seasonalDiscount = roundTo2(t.seasonalDiscount);
	t.specialDiscount = roundTo2(t.specialDiscount);
	t.payable = roundTo2(t.payable);
	return t;
}

Bill::Bill(const string& billId, const string& customerName, const string& orderId,
	const vector<pair<pos::Product, double>>& items, const string& status, const BillAdjustments& adj):
	billId(billId), customerName(customerName), orderId(orderId), products(items), orderStatus(status), date(adj.date), time(adj.time)
{
	if (products.empty())
		throw invalid_argument("Bill must have atleast 1 product");
	BillCalculator calc;
	totals = calc.compute(products, adj.seasonalPercent, adj.specialPercent);
	amountReceived = (adj.amountReceived == 0) ? totals.payable : adj.amountReceived;
	balanceReturned = roundTo2(amountReceived - totals.payable);
}
Bill::Bill(const string& billId, const string& customerName, const string& orderId,
	const vector<pair<pos::Product, double>>& items, const string& status,
	const BillTotals& totals, double amountReceived, double balanceReturned,
	const string& date, const string& time) :
	billId(billId), customerName(customerName), orderId(orderId), products(items),
	orderStatus(status), totals(totals), amountReceived(amountReceived),
	balanceReturned(balanceReturned), date(date), time(time) 
{
	if (products.empty())
		throw invalid_argument("Bill must have atleast 1 product");
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
const vector<pair<pos::Product, double>>& Bill::getProducts() const {
	return products;
}
double Bill::getTotalWithoutTax() const {
	return totals.withoutTax;
}
double Bill::getTotalTax() const {
	return totals.tax;
}
double Bill::getTotalDiscount() const {
	return totals.productDiscount;
}
double Bill::getTotalBill() const {
	return totals.payable;
}
double Bill::getSeasonalDiscount() const {
	return totals.seasonalDiscount;
}
double Bill::getSpecialDiscount() const {
	return totals.specialDiscount;
}
double Bill::getAmountReceived() const {
	return amountReceived;
}
double Bill::getBalanceReturned() const {
	return balanceReturned;
}
const string& Bill::getDate() const {
	return date;
}
const string& Bill::getTime() const {
	return time;
}
const string& Bill::getOrderStatus() const {
	return orderStatus;
}

void Bill::setOrderStatus(const std::string& status) {
	orderStatus = status;
}
} // namespace pos
