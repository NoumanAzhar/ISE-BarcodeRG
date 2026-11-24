#include "Roles.h"

#include "BarcodesPage.h"
#include "CategoriesPage.h"
#include "InventoryPage.h"
#include "OrderPage.h"
#include "OrdersPage.h"
#include "ProductsPage.h"
#include "ReadPage.h"
#include "ReportsPage.h"
#include "SeasonsPage.h"
#include "Workspace.h"
#include "access.h"

namespace Roles {

namespace {

// Adds a page only when the access table (access.h) lets the role use the feature,
// so what each role sees is decided in one place and a new page is one more line below.
template <typename MakePage>
void addIfAllowed(Workspace* workspace, pos::Role role, pos::Action action,
                  const QString& title, IconWidget::Kind icon, MakePage makePage)
{
    if (pos::isAllowed(role, action))
        workspace->addPage(title, icon, makePage());
}

} // namespace

Workspace* buildCashier(pos::AppServices& services, QWidget* parent)
{
    using pos::Action;
    using pos::Role;

    auto* workspace = new Workspace("Cashier", parent);
    addIfAllowed(workspace, Role::Cashier, Action::RecordOrder, OrderPage::kTitle, IconWidget::Order,
                 [&] { return new OrderPage(services); });
    addIfAllowed(workspace, Role::Cashier, Action::EditOrder, "Orders", IconWidget::Orders,
                 [&] { return new OrdersPage(services, OrdersMode::Cashier); });
    addIfAllowed(workspace, Role::Cashier, Action::ViewProduct, "Scan Product", IconWidget::Scan,
                 [&] { return new ReadPage(services, ReadMode::CameraOnly); });
    return workspace;
}

Workspace* buildManager(pos::AppServices& services, QWidget* parent)
{
    using pos::Action;
    using pos::Role;

    auto* workspace = new Workspace("Manager", parent);
    addIfAllowed(workspace, Role::Manager, Action::ManageProducts, "Products", IconWidget::Products,
                 [&] { return new ProductsPage(services); });
    addIfAllowed(workspace, Role::Manager, Action::ViewInventory, "Inventory", IconWidget::Inventory,
                 [&] { return new InventoryPage(services); });
    addIfAllowed(workspace, Role::Manager, Action::ManageCategories, "Categories", IconWidget::Categories,
                 [&] { return new CategoriesPage(services); });
    addIfAllowed(workspace, Role::Manager, Action::ApproveEdit, "Orders", IconWidget::Orders,
                 [&] { return new OrdersPage(services, OrdersMode::Manager); });
    addIfAllowed(workspace, Role::Manager, Action::ManageSeasons, "Discounts", IconWidget::Discount,
                 [&] { return new SeasonsPage(services); });
    addIfAllowed(workspace, Role::Manager, Action::ViewReports, "Reports", IconWidget::Reports,
                 [&] { return new ReportsPage(services); });
    addIfAllowed(workspace, Role::Manager, Action::GenerateBarcode, "Barcodes", IconWidget::Scan,
                 [&] { return new BarcodesPage(services); });
    return workspace;
}

} // namespace Roles
