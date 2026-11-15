#include<iomanip>
#include<fstream>
#include"BillStorage.h"
#include<stdexcept>
using namespace std;
void BillStorage::saveBill(const Bill& bill, const string& filename) {
	ofstream fout(filename, ios::app);
	if (fout.is_open()) {
		const auto& items = bill.getProducts();
		fout << fixed << setprecision(2);
		fout << bill.getBillId() << '|' << bill.getCustomerName() << '|';
		fout << bill.getOrderId() << '|' << items.size() << '|';
		for (int i = 0; i < items.size(); i++) {
			fout << items[i].first.getAcronym() << "|";
			fout << items[i].first.getUnitPrice() << "|";
			fout << items[i].first.getDiscount() << "|";
			fout << items[i].second << "|";
		}
		fout << bill.getTotalWithoutTax() << "|" << bill.getTotalTax() << "|" << bill.getTotalDiscount() << "|";
		fout << bill.getTotalBill() << "|" << bill.getOrderStatus() << "#\n";
	}
	else {
		throw runtime_error("Could not open " + filename);
	}
}