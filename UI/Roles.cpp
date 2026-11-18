#include "Roles.h"

#include "GeneratePage.h"
#include "OrderPage.h"
#include "ProductsPage.h"
#include "ReadPage.h"
#include "Workspace.h"

namespace Roles {

Workspace* buildCashier(pos::AppServices& services, QWidget* parent)
{
    auto* workspace = new Workspace("Cashier", parent);
    workspace->addPage("New Order", IconWidget::Order, new OrderPage(services));
    workspace->addPage("Scan Product", IconWidget::Scan,
                       new ReadPage(services, ReadMode::CameraOnly));
    return workspace;
}

Workspace* buildManager(pos::AppServices& services, QWidget* parent)
{
    auto* workspace = new Workspace("Manager", parent);
    workspace->addPage("Generate", IconWidget::Generate, new GeneratePage);
    workspace->addPage("Read", IconWidget::Scan,
                       new ReadPage(services, ReadMode::ImageAndCamera));
    workspace->addPage("Products", IconWidget::Products, new ProductsPage(services));
    return workspace;
}

} // namespace Roles
