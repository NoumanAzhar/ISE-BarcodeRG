#include "Inventory.h"
#include <cctype>
#include <cmath>
#include <string>
#include <vector>
using namespace std;

namespace pos {

    namespace {

        // rounding noise smaller than this is not a real shortage (0.3 - 0.1 - 0.2 is not below 0)
        const double kStockEpsilon = 1e-9;

        string lower(string text)
        {
            for (size_t i = 0; i < text.length(); i++)
                text[i] = static_cast<char>(tolower(static_cast<unsigned char>(text[i])));
            return text;
        }

        string categoryNameOf(const vector<Category>& categories, const string& categoryId)
        {
            for (size_t i = 0; i < categories.size(); i++)
            {
                if (categories[i].id == categoryId)
                    return categories[i].name;
            }
            return "";
        }

        bool comesBefore(const InventoryRow& a, const InventoryRow& b)
        {
            const string nameA = lower(a.name);
            const string nameB = lower(b.name);
            if (nameA != nameB)
                return nameA < nameB;
            return a.productId < b.productId;
        }

        bool isAllowedQuantity(const Product& p, double quantity)
        {
            if (!isfinite(quantity) || quantity < 0)
                return false;
            if (p.unit == MeasureUnit::Count && quantity != floor(quantity))
                return false;                   // a count product is stocked in whole units
            return true;
        }

        bool storeQuantity(IProductRepository& products, Product product, double quantity)
        {
            if (!isAllowedQuantity(product, quantity))
                return false;
            product.availableQty = quantity;
            return products.update(product);
        }

    } // anonymous namespace

    InventoryService::InventoryService(IProductRepository& products, const ICategoryRepository& categories)
        : products_(products), categories_(categories)
    {
    }

    vector<InventoryRow> InventoryService::list() const
    {
        vector<Category> categories = categories_.all();
        vector<Product> items = products_.all();
        vector<InventoryRow> rows;
        for (size_t i = 0; i < items.size(); i++)
        {
            InventoryRow row;
            row.productId = items[i].id;
            row.acronym = items[i].acronym;
            row.name = items[i].name;
            row.categoryName = categoryNameOf(categories, items[i].categoryId);
            row.unit = items[i].unit;
            row.quantity = items[i].availableQty;
            rows.push_back(row);
        }
        for (size_t i = 1; i < rows.size(); i++)
        {
            InventoryRow current = rows[i];
            size_t j = i;
            while (j > 0 && comesBefore(current, rows[j - 1]))
            {
                rows[j] = rows[j - 1];
                j--;
            }
            rows[j] = current;
        }
        return rows;
    }

    bool InventoryService::setQuantity(const string& productId, double quantity)
    {
        optional<Product> product = products_.findById(productId);
        if (!product)
            return false;
        return storeQuantity(products_, *product, quantity);
    }

    bool InventoryService::addStock(const string& productId, double amount)
    {
        if (!isfinite(amount) || amount <= 0)
            return false;
        optional<Product> product = products_.findById(productId);
        if (!product)
            return false;
        return storeQuantity(products_, *product, product->availableQty + amount);
    }

    bool InventoryService::adjust(const string& productId, double delta)
    {
        optional<Product> product = products_.findById(productId);
        if (!product)
            return false;
        if (delta == 0)
            return true;
        double quantity = product->availableQty + delta;
        if (quantity < 0 && quantity > -kStockEpsilon)
            quantity = 0;
        return storeQuantity(products_, *product, quantity);
    }

} // namespace pos