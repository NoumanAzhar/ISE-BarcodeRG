#pragma once

#include <QWidget>

class Workspace;
namespace pos { class AppServices; }

// The two roles of the app. This is the ONE place that decides which pages a
// role has: adding a page is one addPage() line here, nothing else changes.
namespace Roles {

Workspace* buildCashier(pos::AppServices& services, QWidget* parent);
Workspace* buildManager(pos::AppServices& services, QWidget* parent);

} // namespace Roles
