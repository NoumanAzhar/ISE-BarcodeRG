#pragma once
#include <string>
#include <vector>
#include "Category.h"
#include "Product.h"

namespace pos {

    // One line of the manager's inventory table.
    struct InventoryRow {
        std::string productId, acronym, name, categoryName;
        MeasureUnit unit;
        double quantity;
    };

    class InventoryService {
    public:
        InventoryService(IProductRepository& products, const ICategoryRepository& categories);

        // every product as a row, sorted by name; categoryName is empty if the category is unknown
        std::vector<InventoryRow> list() const;
        // manager update: sets the stock to an exact value, must be >= 0
        bool setQuantity(const std::string& productId, double quantity);
        // manager update: adds to the stock, amount must be > 0
        bool addStock(const std::string& productId, double amount);
        // used by orders and edits (the only place stock changes);
        // false if the product is unknown or the stock would go below 0
        bool adjust(const std::string& productId, double delta);

    private:
        IProductRepository& products_;
        const ICategoryRepository& categories_;
    };

} // namespace pos