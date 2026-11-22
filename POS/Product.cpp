#include "Product.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

/*

Product.txt file

- product id
- product barcode
- product acronym (max 8 characters)
- product name
- product measurement unit (count, weight)
- product unit price
- tax on product (amount in PKR)
- discount on one unit of product
- available quantity
- barcode file name and path

*/

namespace pos {

    // helpers local to this file (IProductRepository etc. are declared in Product.h only)
    namespace {

        string num2str(double n)
        {
            ostringstream out;
            out << fixed << setprecision(2) << n;
            return out.str();
        }

        string formatMoney(double value)
        {
            return num2str(value);
        }

        string formatQuantity(double value)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.10g", value);   // 2 -> "2", 2.5 -> "2.5"
            return buffer;
        }

        Record producttoRecord(const Product& p)
        {
            Record r;
            r.push_back(p.id);
            r.push_back(p.barcode);
            r.push_back(p.acronym);
            r.push_back(p.name);
            r.push_back(p.unit == MeasureUnit::Count ? "count" : "weight");
            r.push_back(num2str(p.unitPrice));
            r.push_back(num2str(p.tax));
            r.push_back(num2str(p.discount));
            r.push_back(formatQuantity(p.availableQty));
            r.push_back(p.barcodeFile);
            r.push_back(p.categoryId);
            return r;
        }

        double toDouble(const string& text)
        {
            size_t used = 0;
            double value = stod(text, &used);
            if (used != text.length())
                throw invalid_argument("extra characters");
            return value;
        }

        bool isNumber(const string& text)
        {
            if (text.empty() || text.length() > 9)
                return false;
            for (size_t i = 0; i < text.length(); i++)
            {
                if (text[i] < '0' || text[i] > '9')
                    return false;
            }
            return true;
        }

        string upper(string text)
        {
            for (size_t i = 0; i < text.length(); i++)
                text[i] = static_cast<char>(toupper(static_cast<unsigned char>(text[i])));
            return text;
        }

        string makeProductId(int number)
        {
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%04d", number);
            return buffer;
        }

        optional<Product> makeProduct(const Record& row)
        {
            if (row.size() != 10 && row.size() != 11)
            {
                return nullopt;
            }
            string id = row[0], barcode = row[1], name = row[3], acronym = row[2], path = row[9];
            string unit = row[4];
            for (size_t i = 0; i < unit.length(); i++)
                unit[i] = static_cast<char>(tolower(static_cast<unsigned char>(unit[i])));
            if (unit != "count" && unit != "weight")
            {
                return nullopt;
            }
            if (id.empty() || acronym.length() > kMaxAcronym)
            {
                return nullopt;
            }
            double tax, discount, price, quantity;
            try
            {
                price = toDouble(row[5]);
                tax = toDouble(row[6]);
                discount = toDouble(row[7]);
                quantity = toDouble(row[8]);
            }
            catch (...)
            {
                return nullopt;
            }
            if (price < 0 || tax < 0 || discount < 0 || quantity < 0)
            {
                return nullopt;
            }
            MeasureUnit measureUnit = (unit == "count") ? MeasureUnit::Count : MeasureUnit::Weight;
            Product product(id, barcode, acronym, name, measureUnit, price, tax, discount, quantity, path);
            if (row.size() == 11)
                product.categoryId = row[10];
            return product;
        }

    } // anonymous namespace

    bool isValidAcronym(const string& acronym)
    {
        if (acronym.length() < kMinAcronym || acronym.length() > kMaxAcronym)
            return false;
        bool hasLetter = false;
        for (size_t i = 0; i < acronym.length(); i++)
        {
            const char c = acronym[i];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                hasLetter = true;
            else if (c < '0' || c > '9')
                return false;
        }
        return hasLetter;
    }

    // ---- FileProductRepository ----

    FileProductRepository::FileProductRepository(string path)
    {
        this->path_ = path;
    }

    optional<Product> FileProductRepository::findByBarcode(const string& barcode) const
    {
        if (barcode == "")
            return nullopt;
        vector<Record> data = readRecords(path_);
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() >= 2)
                if ((data[i])[1] == barcode)
                    return makeProduct(data[i]);
        }
        return nullopt;
    }

    optional<Product> FileProductRepository::findById(const string& id) const
    {
        vector<Record> data = readRecords(path_);
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() >= 2)
                if ((data[i])[0] == id)
                    return makeProduct(data[i]);
        }
        return nullopt;
    }

    vector<Product> FileProductRepository::all() const
    {
        warnings_.clear();
        vector<Record> data = readRecords(path_);
        vector<Product> res;
        for (size_t i = 0; i < data.size(); i++)
        {
            auto p = makeProduct(data[i]);
            if (p.has_value())
                res.push_back(*p);
            else
                warnings_.push_back("record " + to_string(i + 1) + ": expected 10 or 11 valid fields");
        }
        return res;
    }

    bool FileProductRepository::update(const Product& product)
    {
        vector<Record> data = readRecords(path_);
        for (size_t i = 0; i < data.size(); i++)
        {
            if (!data[i].empty() && data[i][0] == product.id)
            {
                const optional<Product> stored = makeProduct(data[i]);
                if (stored)
                {
                    if (stored->unit != product.unit)
                        return false;                       // the measurement unit never changes
                    if (upper(stored->acronym) != upper(product.acronym))
                    {
                        if (!isValidAcronym(product.acronym))
                            return false;
                        for (size_t j = 0; j < data.size(); j++)
                            if (j != i && data[j].size() > 2 && upper(data[j][2]) == upper(product.acronym))
                                return false;               // acronym already used
                    }
                }
                data[i] = producttoRecord(product);
                return writeRecords(path_, data);
            }
        }
        return false;
    }

    bool FileProductRepository::add(const Product& newProduct)
    {
        vector<Record> data = readRecords(path_);
        Product product = newProduct;
        if (!isValidAcronym(product.acronym))
            return false;
        product.acronym = upper(product.acronym);
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() > 2 && upper(data[i][2]) == product.acronym)
                return false;                               // acronym already used
        }
        if (product.id.empty())
        {
            product.id = nextId();
        }
        else
        {
            for (size_t i = 0; i < data.size(); i++)
            {
                if (!data[i].empty() && data[i][0] == product.id)
                    return false;
            }
        }
        data.push_back(producttoRecord(product));
        return writeRecords(path_, data);
    }

    optional<Product> FileProductRepository::findByAcronym(const string& acronym) const
    {
        const string wanted = upper(acronym);
        vector<Record> data = readRecords(path_);
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() > 2 && upper(data[i][2]) == wanted)
                return makeProduct(data[i]);
        }
        return nullopt;
    }

    bool FileProductRepository::remove(const string& id)
    {
        vector<Record> data = readRecords(path_);
        vector<Record> kept;
        bool found = false;
        for (size_t i = 0; i < data.size(); i++)
        {
            if (!data[i].empty() && data[i][0] == id)
            {
                found = true;
                continue;
            }
            kept.push_back(data[i]);
        }
        if (!found)
            return false;
        return writeRecords(path_, kept);
    }

    bool FileProductRepository::assignIDs()
    {
        vector<Record> data = readRecords(path_);
        int maxId = 0;
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() >= 10 && isNumber(data[i][0]) && stoi(data[i][0]) > maxId)
                maxId = stoi(data[i][0]);
        }
        bool changed = false;
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() < 10 || !data[i][0].empty())
                continue;
            data[i][0] = makeProductId(++maxId);
            changed = true;
        }
        if (!changed)
            return true;
        return writeRecords(path_, data);
    }
    string FileProductRepository::nextId() const
    {
        vector<Record> data = readRecords(path_);
        int maxId = 0;
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() >= 10 && isNumber(data[i][0]))
            {
                try {
                    int val = stoi(data[i][0]);
                    if (val > maxId) maxId = val;
                }
                catch (...) {}
            }
        }
        return makeProductId(maxId + 1);
    }

    const vector<string>& FileProductRepository::warnings() const
    {
        return warnings_;
    }

    // ---- ProductCatalog ----

    ProductCatalog::ProductCatalog(const IProductRepository& repo) : repo_(repo)
    {
    }

    optional<Product> ProductCatalog::lookup(const string& barcode) const
    {
        if (barcode.length() != (unsigned int)kBarcodeDigits)
            return nullopt;
        for (size_t i = 0; i < barcode.length(); i++)
        {
            if (barcode[i] < '0' || barcode[i] > '9')
                return nullopt;
        }
        return repo_.findByBarcode(barcode);
    }

    // Multi-line, human-readable text for the Description button and scan screen.
    string describeProduct(const Product& p)
    {
        string text;
        text += "Name: " + p.name + "\n";
        text += "Acronym: " + p.acronym + "\n";
        text += "Product ID: " + p.id + "\n";
        text += "Barcode: " + (p.barcode == "" ? string("not assigned") : p.barcode) + "\n";
        text += string("Unit: ") + (p.unit == MeasureUnit::Count ? "count" : "weight") + "\n";
        text += "Price: PKR " + formatMoney(p.unitPrice) + "\n";
        text += "Tax (per unit): PKR " + formatMoney(p.tax) + "\n";
        text += "Discount (per unit): PKR " + formatMoney(p.discount) + "\n";
        text += "Available: " + formatQuantity(p.availableQty);
        return text;
    }

    // ---- ProductService ----

    namespace {

        ProductResult validateProduct(const Product& p, const ICategoryRepository& categories)
        {
            ProductResult res;
            res.product = p;
            if (p.name.find_first_not_of(' ') == string::npos)
            {
                res.error = ProductError::EmptyName;
                res.message = "product name is empty";
            }
            else if (!isValidAcronym(p.acronym))
            {
                res.error = ProductError::BadAcronym;
                res.message = "acronym must be up to 8 letters or digits and contain a letter";
            }
            else if (p.unitPrice <= 0 || p.tax < 0)
            {
                res.error = ProductError::BadPrice;
                res.message = "price must be above 0 and tax cannot be negative";
            }
            else if (p.discount < 0 || p.discount > p.unitPrice)
            {
                res.error = ProductError::BadDiscount;
                res.message = "discount must be between 0 and the unit price";
            }
            else if (p.availableQty < 0 || (p.unit == MeasureUnit::Count && p.availableQty != floor(p.availableQty)))
            {
                res.error = ProductError::BadStock;
                res.message = "stock cannot be negative, and a count product needs a whole number";
            }
            else if (!categories.findById(p.categoryId))
            {
                res.error = ProductError::UnknownCategory;
                res.message = "category does not exist";
            }
            return res;
        }

    } // anonymous namespace

    ProductService::ProductService(IProductRepository& products, const ICategoryRepository& categories)
        : products_(products), categories_(categories)
    {
    }

    ProductResult ProductService::add(Product p)
    {
        ProductResult res = validateProduct(p, categories_);
        if (!res.ok())
            return res;
        vector<Product> existing = products_.all();
        for (size_t i = 0; i < existing.size(); i++)
        {
            if (upper(existing[i].acronym) == upper(p.acronym))
            {
                res.error = ProductError::BadAcronym;
                res.message = "acronym already used";
                return res;
            }
        }
        p.acronym = upper(p.acronym);
        p.id = products_.nextId();
        p.barcode = "";
        p.barcodeFile = "";
        if (!products_.add(p))
        {
            res.error = ProductError::SaveFailed;
            res.message = "could not save the product";
            return res;
        }
        res.product = p;
        return res;
    }

    ProductResult ProductService::edit(const Product& p)
    {
        ProductResult res;
        res.product = p;
        optional<Product> stored = products_.findById(p.id);
        if (!stored)
        {
            res.error = ProductError::NotFound;
            res.message = "product not found";
            return res;
        }
        res = validateProduct(p, categories_);
        if (!res.ok())
            return res;
        if (stored->unit != p.unit)
        {
            res.error = ProductError::SaveFailed;
            res.message = "the measurement unit cannot be changed";
            return res;
        }
        vector<Product> existing = products_.all();
        for (size_t i = 0; i < existing.size(); i++)
        {
            if (existing[i].id != p.id && upper(existing[i].acronym) == upper(p.acronym))
            {
                res.error = ProductError::BadAcronym;
                res.message = "acronym already used";
                return res;
            }
        }
        Product updated = p;
        updated.acronym = upper(p.acronym);
        updated.barcode = stored->barcode;
        updated.barcodeFile = stored->barcodeFile;
        if (!products_.update(updated))
        {
            res.error = ProductError::SaveFailed;
            res.message = "could not save the product";
            return res;
        }
        res.product = updated;
        return res;
    }
}