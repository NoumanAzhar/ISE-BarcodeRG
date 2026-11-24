#pragma once

#include "BillGenerator.h"
#include "BillStorage.h"
#include "CheckoutService.h"
#include "Category.h"
#include "Inventory.h"
#include "Product.h"
#include "barcode_assign.h"
#include "bill_pdf.h"
#include "order.h"
#include "order_edit.h"
#include "order_service.h"
#include "report.h"
#include "season.h"

namespace pos {

    // Composition root of the POS backend: creates every repository and service
    // once, wired together, so the GUI never builds or owns backend objects itself.
    // No Qt in here. Members are declared in dependency order (they hold references
    // to the ones above them), so do NOT reorder them.
    class AppServices {
    public:
        // Creates data/, data/barcodes/ and data/bills/ if missing, gives every product
        // without an id the next free one, then fills every barcode gap (assignAll).
        AppServices();

        AppServices(const AppServices&) = delete;
        AppServices& operator=(const AppServices&) = delete;

        // repositories
        FileProductRepository& products() { return products_; }
        FileCategoryRepository& categories() { return categories_; }
        IOrderRepository& orders() { return orders_; }
        IBillRepository& bills() { return bills_; }
        IApprovalRepository& approvals() { return approvals_; }
        // services
        CategoryService& categoryService() { return categoryService_; }
        ProductService& productService() { return productService_; }
        InventoryService& inventory() { return inventory_; }
        SeasonService& seasons() { return seasonService_; }
        OrderEditService& orderEdit() { return orderEdit_; }
        OrderService& order() { return order_; }              // the draft order being recorded
        OrderHoldService& hold() { return hold_; }
        CheckoutService& checkout() { return checkout_; }
        ReportService& reports() { return reports_; }
        IBillExporter& exporter() { return exporter_; }
        ProductCatalog& catalog() { return catalog_; }
        BarcodeAssigner& assigner() { return assigner_; }

        // What assignAll did at start-up (messages about barcodes that were reported).
        const AssignReport& startupBarcodeReport() const { return startupReport_; }

    private:
        FileProductRepository products_;
        FileCategoryRepository categories_;
        FileSeasonRepository seasonRepo_;
        FileOrderRepository orders_;
        FileBillRepository bills_;
        FileApprovalRepository approvals_;
        SystemClock clock_;
        CategoryService categoryService_;
        ProductService productService_;
        InventoryService inventory_;
        SeasonService seasonService_;
        BillGenerator billGenerator_;
        OrderEditService orderEdit_;
        CheckoutService checkout_;
        OrderService order_;
        OrderHoldService hold_;
        ReportService reports_;
        PdfBillExporter exporter_;
        ProductCatalog catalog_;
        BarcodeAssigner assigner_;
        AssignReport startupReport_;
    };

} // namespace pos
