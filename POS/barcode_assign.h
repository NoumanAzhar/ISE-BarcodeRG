#pragma once

#include <string>
#include <vector>

#include "Data_Files.h"
#include "Product.h"
#include "category.h"

namespace pos {
	struct AssignReport {
		int assigned = 0;
		int kept = 0;
		int failed = 0;
		std::vector<std::string> messages;
	};

	class BarcodeAssigner {
	public:
		BarcodeAssigner(IProductRepository& products, const IcategoryRepository& categories, std::string imageDir = kBarcodeDir);
		// A barcode is kept if it is valid, unique and starts with the product's
		// category id. Others get categoryId + 4 random digits, unique.
		// Draws <imageDir>/<barcode>.png
		AssignReport assignAll();
	private:
		IProductRepository& products_;
		const ICategoryRepository& categories_;
		std::string imageDir_;
	};
}