#pragma once
#include<string>
#include"Bill.h"
class BillGenerator {
	static void loadBillCounter();
public:
	Bill generateBill(const std::string& orderId);
};
