#pragma once

#include "BillGenerator.h"
#include "CheckoutService.h"
#include "Product.h"
#include "barcode_assign.h"
#include "order.h"
#include "order_service.h"

namespace pos {

// Composition root of the POS backend: creates the repositories and services
// once, wired together, so the GUI never builds or owns backend objects itself.
// No Qt in here. Members are declared in dependency order (they hold references
// to the ones above them), so do not reorder them.
class AppServices {
public:
    // Creates data/ and data/barcodes/ if missing and gives every product
    // without an id the next free one.
    AppServices();

    AppServices(const AppServices&) = delete;
    AppServices& operator=(const AppServices&) = delete;

    FileProductRepository& products() { return products_; }
    IOrderRepository& orders() { return orders_; }
    OrderService& order() { return order_; }          // the draft order being recorded
    CheckoutService& checkout() { return checkout_; }
    ProductCatalog& catalog() { return catalog_; }
    BarcodeAssigner& assigner() { return assigner_; }

private:
    FileProductRepository products_;
    FileOrderRepository orders_;
    SystemClock clock_;
    BillGenerator billGenerator_;
    OrderService order_;
    CheckoutService checkout_;
    ProductCatalog catalog_;
    BarcodeAssigner assigner_;
};

} // namespace pos
