#pragma once
#include<iostream>
#include<opencv2/opencv.hpp>
#include<vector>
#include<optional>
using namespace std;

// product.h
enum class MeasureUnit { Count, Weight };
struct Product {
	std::string id, barcode, acronym, name;
	MeasureUnit unit;
	double unitPrice, tax, discount, availableQty; std::string barcodeFile;
};
class IProductRepository 
{
public: 
	virtual ~IProductRepository() = default;
	virtual std::optional<Product> findByBarcode(const std::string& barcode) const = 0;
	virtual std::optional<Product> findById(const std::string& id) const = 0;
	virtual std::vector<Product> all() const = 0;
	virtual bool update(const Product& product) = 0;
}; // changes one product (stock, barcode, image path)
class FileProductRepository : public IProductRepository
{
public:
	explicit FileProductRepository(std::string path);

};
class ProductCatalog
{
public:
	explicit ProductCatalog(const IProductRepository& repo);
	  // nullopt if not 6 digits or unknown
	std::optional<Product> lookup(const std::string& barcode) const;
};
std::string describeProduct(const Product& p);
