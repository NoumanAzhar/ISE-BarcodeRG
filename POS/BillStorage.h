#pragma once
#include<string>
#include"Bill.h"
class BillStorage {
public:
	static void saveBill(const Bill& bill, const std::string& filename="Bill.txt");
};