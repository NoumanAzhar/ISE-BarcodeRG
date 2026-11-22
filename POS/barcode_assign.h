#pragma once

#include <string>
#include <vector>

#include "Product.h"

namespace pos {

    struct AssignReport {
        int assigned = 0;
        int kept = 0;
        int failed = 0;
        std::vector<std::string> messages;
    };

    class BarcodeAssigner {
    public:
        explicit BarcodeAssigner(IProductRepository& products,
            std::string imageDir = "data/barcodes");

        // empty, "-", not 6 digits, leading 0 or duplicate barcode = unassigned;
        // draws <imageDir>/<barcode>.png
        AssignReport assignAll();

    private:
        IProductRepository& products_;
        std::string imageDir_;
    };

} 