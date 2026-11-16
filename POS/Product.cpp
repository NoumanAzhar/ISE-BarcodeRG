#include "Product.h"
#include<fstream>
#include<string>
#include<vector>
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

    class IProductRepository {
    public:
        virtual ~IProductRepository() = default;
        // nullopt if no product has this barcode
        virtual std::optional<Product> findByBarcode(const std::string& barcode) const = 0;
        // nullopt if no product has this id
        virtual std::optional<Product> findById(const std::string& id) const = 0;
        // every valid product, in file order
        virtual std::vector<Product> all() const = 0;
        // replaces the product with the same id (stock, barcode, image path);
        // false if the id is not found or the write fails
        virtual bool update(const Product& product) = 0;
    };

    string num2str(double n)
    {
        ostringstream out;
        out << fixed << setprecision(2) << n;
        return out.str();
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
        r.push_back(num2str(p.availableQty));
        r.push_back(p.barcodeFile);
        return r;
    }

    class FileProductRepository : public IProductRepository
    {
    private:
        std::string path_;
    public:

        explicit FileProductRepository(std::string path)
        {
            this->path_ = path;
        }

        double toDouble(const string& text)const
        {
            size_t used = 0;
            double value = stod(text, &used);
            if (used != text.length())
                throw invalid_argument("extra characters");
            return value;
        }

        optional<Product> makeProduct(const Record& row)const
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
            MeasureUnit measureUnit =(unit == "count") ? MeasureUnit::Count : MeasureUnit::Weight;
            return Product(id, barcode, acronym, name, measureUnit, price, tax, discount, quantity, path);
        }

        optional<Product> findByBarcode(const std::string& barcode) const override
        {
            vector<Record> data = readRecords(path_);
            for (int i = 0; i < data.size(); i++)
            {
                if (data[i].size() >= 2)
                    if ((data[i])[1] == barcode)
                        return makeProduct(data[i]);
            }
            return nullopt;
        }

        optional<Product> findById(const std::string& id) const override
        {
            vector<Record> data = readRecords(path_);
            for (int i = 0; i < data.size(); i++)
            {

                if (data[i].size() >= 2)
                    if ((data[i])[0] == id)
                        return makeProduct(data[i]);
            }
            return nullopt;
        }

        vector<Product> all() const override
        {
            vector<Record> data = readRecords(path_);
            vector<Product> res;
            for (int i = 0; i < data.size(); i++)
            {

                auto p = makeProduct(data[i]);
                if (p.has_value())
                    res.push_back(*p);
            }
            return res;
        }

        bool update(const Product& product) override
        {
            vector<Record> data = readRecords(path_);
            for (int i = 0; i < data.size(); i++)
            {
                if (!data[i].empty() && data[i][0] == product.id)
                {
                    data[i] = producttoRecord(product);
                    return writeRecords(path_, data);
                }
            }
            return 0;
        }
    };

    class ProductCatalog
    {
    public:
        explicit ProductCatalog(const IProductRepository& repo):repo_(repo){}

        // nullopt if the barcode is not exactly kBarcodeDigits digits, or unknown
        optional<Product> lookup(const std::string& barcode) const
        {
            if (barcode.length() != (unsigned int)kBarcodeDigits)
                return nullopt;
            for (int i = 0; i < barcode.length(); i++)
            {
                if (barcode[i] < '0' || barcode[i] > '9')
                    return nullopt;
            }
            return repo_.findByBarcode(barcode);
        }

    private:
        const IProductRepository& repo_;
    };

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
        text += "Available: " + formatQuantity(p);
        return text;
    }
}