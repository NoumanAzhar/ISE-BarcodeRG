#include<Product.h>
#include<fstream>

using namepsace std;

/*
struct Product {
	std::string id, barcode, acronym, name;
	MeasureUnit unit;
	double unitPrice, tax, discount, availableQty; std::string barcodeFile;
};


4. Product.txt file shall have one row for each product. Each row shall have a # as an end of line market.
Each line shall have following information about the product in the exact order, separated  by a space:
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

    // ---- Layer 1: model ----
    enum class MeasureUnit { Count, Weight };

    struct Product {
        std::string id, barcode, acronym, name;
        MeasureUnit unit;
        double unitPrice, tax, discount, availableQty;
        std::string barcodeFile;
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
    };

    // ---- Layer 2: Product.txt implementation ----
    class FileProductRepository : public IProductRepository {
    public:
        explicit FileProductRepository(std::string path);

        std::optional<Product> findByBarcode(const std::string& barcode) const override;
        std::optional<Product> findById(const std::string& id) const override;
        std::vector<Product> all() const override;
        bool update(const Product& product) override;

        // (added, not in the shared interface) messages about malformed lines
        // from the most recent read, e.g. "line 3: expected 10 fields".
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

} // namespace pos