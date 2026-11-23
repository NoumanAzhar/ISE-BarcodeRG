#pragma once
#include <cctype>
#include <optional>
#include <string>
#include <vector>
#include "Data_Files.h"
#include "Category.h"


namespace pos {

    // ---- Layer 1: model ----
    enum class MeasureUnit { Count, Weight };

    struct Product {
        std::string id, barcode, acronym, name;
        MeasureUnit unit;
        double unitPrice, tax, discount, availableQty;
        std::string barcodeFile;
        std::string categoryId;
        Product()
        {
            id = "";
            barcode = "";
            acronym = "";
            name = "";
            unit = MeasureUnit::Weight;
            barcodeFile = "";
            unitPrice = 0.0;
            tax = 0.0;
            discount = 0.0;
            availableQty = 0.0;
            categoryId = "";
        }
        Product(std::string id, std::string barcode, std::string acronym, std::string name, std::string unit, double unitPrice, double tax,
            double discount, double availableQty, std::string barcodeFile)
        {
            this->id = id; this->barcode = barcode; this->acronym = acronym; this->name = name; this->barcodeFile = barcodeFile;
            this->unitPrice = unitPrice; this->tax = tax; this->discount = discount; this->availableQty = availableQty;

            // lower-case copy of the unit text (the old code appended to an undeclared variable 'type')
            std::string lowered;
            for (size_t i = 0; i < unit.length(); i++)
            {
                lowered += static_cast<char>(tolower(static_cast<unsigned char>(unit[i])));
            }

            if (lowered == "count")
                this->unit = MeasureUnit::Count;
            else
                this->unit = MeasureUnit::Weight;

        }
        Product(std::string id, std::string barcode, std::string acronym, std::string name, MeasureUnit unit, double unitPrice, double tax,
            double discount, double availableQty, std::string barcodeFile)
        {
            this->id = id; this->barcode = barcode; this->acronym = acronym; this->name = name; this->barcodeFile = barcodeFile;
            this->unitPrice = unitPrice; this->tax = tax; this->discount = discount; this->availableQty = availableQty;
            this->unit = unit;
        }
    };

    // ---- Layer 2: storage interface ----
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
        // appends a new product; if id is empty, assigns next 4-digit sequence id;
        // returns false if id already exists or write fails
        virtual bool add(const Product& product) = 0;
        // nullopt if no product has this acronym (case-insensitive)
        virtual std::optional<Product> findByAcronym(const std::string& acronym) const = 0;
        // deletes the product with this id; false if not found or the write fails
        virtual bool remove(const std::string& id) = 0;
        virtual std::string nextId() const = 0;
    };

    // 2 to 8 characters, letters and digits only, at least one letter.
    bool isValidAcronym(const std::string& acronym);

    // ---- Layer 2: Product.txt implementation ----
    class FileProductRepository : public IProductRepository {
    public:
        explicit FileProductRepository(std::string path);

        std::optional<Product> findByBarcode(const std::string& barcode) const override;
        std::optional<Product> findById(const std::string& id) const override;
        std::vector<Product> all() const override;
        bool update(const Product& product) override;
        bool add(const Product& product) override;
        std::string nextId() const override;
        std::optional<Product> findByAcronym(const std::string& acronym) const override;
        bool remove(const std::string& id) override;
        bool assignIDs();

        // (added, not in the shared interface) messages about malformed lines
        // from the most recent all(), e.g. "record 3: expected 10 valid fields".
        const std::vector<std::string>& warnings() const;

    private:
        std::string path_;
        mutable std::vector<std::string> warnings_;  // mutable: all() is const
    };

    // ---- Layer 3-facing helpers ----
    class ProductCatalog {
    public:
        explicit ProductCatalog(const IProductRepository& repo);
        // nullopt if the barcode is not exactly kBarcodeDigits digits, or unknown
        std::optional<Product> lookup(const std::string& barcode) const;

    private:
        const IProductRepository& repo_;
    };

    // Multi-line, human-readable text for the Description button and scan screen.
    std::string describeProduct(const Product& p);

    // ---- Layer 3: rules ----
    enum class ProductError {
        None, EmptyName, BadAcronym, BadPrice, BadDiscount, BadStock, UnknownCategory, NotFound, SaveFailed
    };

    struct ProductResult {
        ProductError error = ProductError::None;
        std::string message;
        Product product;
        bool ok() const { return error == ProductError::None; }
    };

    class ProductService {
    public:
        ProductService(IProductRepository& products, const ICategoryRepository& categories);

        // validates, assigns the id (barcode stays empty until BarcodeAssigner runs)
        ProductResult add(Product p);
        // by id; barcode and image path are kept
        ProductResult edit(const Product& p);

    private:
        IProductRepository& products_;
        const ICategoryRepository& categories_;
    };

} // namespace pos