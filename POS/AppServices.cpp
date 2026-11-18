#include "AppServices.h"

#include "data_files.h"

#include <filesystem>
#include <system_error>

namespace pos {
namespace {

constexpr const char* kBarcodeDir = "data/barcodes";

void ensureDataFolders()
{
    std::error_code ignored;   // a failure shows up later as an empty product list
    std::filesystem::create_directories(kBarcodeDir, ignored);
}

} // namespace

AppServices::AppServices()
    : products_(kProductFile),
      orders_(kOrderFile, kOrderDetailFile),
      billGenerator_(orders_, products_),
      order_(products_),
      checkout_(orders_, products_, clock_, billGenerator_),
      catalog_(products_),
      assigner_(products_, kBarcodeDir)
{
    ensureDataFolders();
    products_.assignIDs();
}

} // namespace pos
