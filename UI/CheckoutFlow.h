#pragma once

#include <QString>

class QWidget;
namespace pos { class AppServices; }

// Payment dialog -> checkout -> bill dialog for the current draft order.
// Works for a new order, a resumed on-hold order and an edited paid order.
// Not a widget: the order page calls run() and shows the outcome in its banner.
class CheckoutFlow {
public:
    enum class Result { Cancelled, Saved, SavedWithWarning, Failed };
    struct Outcome { Result result; QString message; };   // message is ready to show in a Banner

    explicit CheckoutFlow(pos::AppServices& services);

    // The draft order is reset only for Saved and SavedWithWarning.
    Outcome run(const QString& customerName, const QString& customerContact, QWidget* parent);

private:
    pos::AppServices& m_services;
};
