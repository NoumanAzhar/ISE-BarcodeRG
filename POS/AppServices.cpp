#include "AppServices.h"

#include "Data_Files.h"

#include <filesystem>
#include <system_error>

namespace pos {
    namespace {

        constexpr const char* kDataDir = "data";

        void ensureDataFolders()
        {
            std::error_code ignored;   // a failure shows up later as an empty product list
            std::filesystem::create_directories(kDataDir, ignored);
            std::filesystem::create_directories(kBarcodeImageDir, ignored);
            std::filesystem::create_directories(kBillPdfDir, ignored);
        }

    } // namespace

    AppServices::AppServices()
        : products_(kProductFile),
        categories_(kCategoryFile),
        seasonRepo_(kSeasonFile),
        orders_(kOrderFile, kOrderDetailFile),
        bills_(kBillFile, kBillDetailFile),
        categoryService_(categories_),
        productService_(products_, categories_),
        inventory_(products_, categories_),
        seasonService_(seasonRepo_),
        billGenerator_(orders_, products_, bills_),
        editPolicy_(clock_),
        orderEdit_(orders_, bills_, editPolicy_),
        checkout_(orders_, products_, inventory_, seasonService_, orderEdit_, clock_, billGenerator_),
        order_(products_),
        hold_(order_, orders_, clock_),
        reports_(orders_, bills_, products_),
        exporter_(&orders_),
        catalog_(products_),
        assigner_(products_, categories_, kBarcodeImageDir)
    {
        ensureDataFolders();
        products_.assignIDs();
        startupReport_ = assigner_.assignAll();
    }

} // namespace pos