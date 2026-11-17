#pragma once
#include<string>
#include"Bill.h"
#include"order.h"
#include"Product.h"
class BillGenerator {
	const pos::IOrderRepository& orders_;
	const pos::IProductRepository& products_;
	static void loadBillCounter();
public:
	BillGenerator(const pos::IOrderRepository& orders, const pos::IProductRepository& products);
	Bill generateBill(const std::string& orderId);
};
