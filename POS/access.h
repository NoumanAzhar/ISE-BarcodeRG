#pragma once

namespace pos {

    enum class Role { Cashier, Manager };

    // Add a new action here AND one line in kAccessTable (access.cpp).
    enum class Action {
        RecordOrder, HoldOrder, ResumeOrder, EditOrder, RequestApproval, ApproveEdit,
        PrintBill, ViewProduct, ViewInventory, UpdateInventory, ManageProducts,
        ManageCategories, GenerateBarcode, ManageSeasons, ViewReports
    };

    bool isAllowed(Role role, Action action);

} // namespace pos
