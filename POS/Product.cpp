#include "Product.h"
#include <cctype>
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

        string makeProductId(int number)
        {
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%04d", number);
            return buffer;
        }

        optional<Product> makeProduct(const Record& row)
        {
            if (row.size() != 10)
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
            return Product(id, barcode, acronym, name, measureUnit, price, tax, discount, quantity, path);
        }

    } // anonymous namespace

    // ---- FileProductRepository ----

    FileProductRepository::FileProductRepository(string path)
    {
        this->path_ = path;
    }

    optional<Product> FileProductRepository::findByBarcode(const string& barcode) const
    {
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
                warnings_.push_back("record " + to_string(i + 1) + ": expected 10 valid fields");
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
                data[i] = producttoRecord(product);
                return writeRecords(path_, data);
            }
        }
        return false;
    }

    bool FileProductRepository::assignIDs()
    {
        vector<Record> data = readRecords(path_);
        int maxId = 0;
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() == 10 && isNumber(data[i][0]) && stoi(data[i][0]) > maxId)
                maxId = stoi(data[i][0]);
        }
        bool changed = false;
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].size() != 10 || !data[i][0].empty())
                continue;
            data[i][0] = makeProductId(++maxId);
            changed = true;
        }
        if (!changed)
            return true;
        return writeRecords(path_, data);
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
}
