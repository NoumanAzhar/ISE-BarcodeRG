#include "access.h"
#include <cstddef>

namespace pos {
    namespace {

        struct AccessRow { Action action; bool cashier; bool manager; };

        // The one table that says who may do what. Same order as the Action enum.
        constexpr AccessRow kAccessTable[] = {
            //  action                    cashier manager
            { Action::RecordOrder,        true,   false },
            { Action::HoldOrder,          true,   false },
            { Action::ResumeOrder,        true,   false },
            { Action::EditOrder,          true,   false },
            { Action::RequestApproval,    true,   false },
            { Action::ApproveEdit,        false,  true  },
            { Action::PrintBill,          true,   true  },   // requirement 7: both
            { Action::ViewProduct,        true,   true  },
            { Action::ViewInventory,      false,  true  },
            { Action::UpdateInventory,    false,  true  },
            { Action::ManageProducts,     false,  true  },
            { Action::ManageCategories,   false,  true  },
            { Action::GenerateBarcode,    false,  true  },
            { Action::ManageSeasons,      false,  true  },
            { Action::ViewReports,        false,  true  },
        };

    } // namespace

    bool isAllowed(Role role, Action action)
    {
        for (const AccessRow& row : kAccessTable) {
            if (row.action == action)
                return role == Role::Manager ? row.manager : row.cashier;
        }
        return false;   // unknown action: deny
    }

} // namespace pos
