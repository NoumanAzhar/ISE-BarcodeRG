#pragma once
#include<string>
#include"Bill.h"
#include"data_files.h"
class BillStorage {
public:
	static void saveBill(const Bill& bill, const std::string& filename = pos::kBillFile);
};
