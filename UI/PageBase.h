#pragma once

#include <QString>
#include <QWidget>

// Base class of every screen that lives inside a Workspace (rail + page).
// The Workspace calls these hooks; pages never talk to the Workspace.
// A page does not draw its own title (the Workspace shows it).
class PageBase : public QWidget {
    Q_OBJECT
public:
    using QWidget::QWidget;

    // Return false when the page asked the user (e.g. "discard this order?")
    // and they chose to stay.
    virtual bool canLeave() { return true; }

    virtual void onActivated() {}     // the page became visible
    virtual void onDeactivated() {}   // the page is about to be hidden
    virtual void resetState() {}      // the role is being left: drop typed text, results, selection

signals:
    // Asks the workspace to show the page with this title (e.g. after an order was
    // opened for editing). The page does not know the Workspace; it only names a title.
    void openPageRequested(const QString& pageTitle);

    // The page wants another name on its rail button and header while it is in a special
    // state (e.g. "Edit Order"). An empty text goes back to the normal title.
    void displayTitleChanged(const QString& title);
};
