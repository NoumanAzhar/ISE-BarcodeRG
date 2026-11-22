#pragma once
#include<string>
#include"Bill.h"
#include"BillStorage.h"
#include"order.h"
#include"Product.h"
class BillGenerator {
	const pos::IOrderRepository& orders_;
	const pos::IProductRepository& products_;
	IBillRepository& bills_;
public:
	BillGenerator(const pos::IOrderRepository& orders, const pos::IProductRepository& products, IBillRepository& bills);
	Bill generateBill(const std::string& orderId, const BillAdjustments& adj);
};
