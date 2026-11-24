#pragma once

#include <QString>

namespace pos { class AppServices; struct Product; }

// Glue between the screens and the services for the things more than one
// screen does (holding, resuming and editing orders, season and category
// lookups). Pages call these; they hold no state of their own.
namespace PosActions {

extern const QString kWalkIn;     // customer name when none was typed

struct Result {
    bool ok = false;
    QString message;              // ready to show in a Banner
    QString orderId;
};

// True while a paid order is open as the draft (the cashier is editing it).
bool isEditingPaid(pos::AppServices& services);

// Season discount that applies to a bill made today (0 when none).
double seasonPercentToday(pos::AppServices& services);

// Name of the product's category; empty when it has none.
QString categoryName(pos::AppServices& services, const pos::Product& product);

// Puts the draft on hold (keeping a typed name or contact) and starts a fresh draft.
Result holdDraft(pos::AppServices& services, const QString& name, const QString& contact);

// Opens an on-hold order as the draft. A draft that already has items is put on
// hold first, so nothing is lost.
Result resumeHeld(pos::AppServices& services, const QString& orderId);

// Opens a paid order for editing, if the edit rules allow it right now.
Result openForEdit(pos::AppServices& services, const QString& orderId);

} // namespace PosActions
