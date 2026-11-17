#pragma once

#include "PageBase.h"
#include "Widgets.h"

#include <QString>
#include <QWidget>

#include <vector>

class PageHeader;
class QStackedWidget;
class QVBoxLayout;
class RailItem;

// One role's working area: a rail of page buttons on the left, the chosen page
// (with its title) on the right. It knows nothing about the pages it holds, so
// a new page is one addPage() call (see Roles.cpp).
class Workspace : public QWidget {
    Q_OBJECT
public:
    explicit Workspace(const QString& roleName, QWidget* parent = nullptr);

    const QString& roleName() const { return m_roleName; }
    void addPage(const QString& title, IconWidget::Kind icon, PageBase* page);

    bool canLeave();     // asks the visible page whether it may be left
    void enter();        // the workspace became visible: activates the visible page
    void leave();        // the workspace is being hidden: deactivates the visible page

private:
    void select(int index);

    QString m_roleName;
    QWidget* m_rail = nullptr;
    QVBoxLayout* m_railLayout = nullptr;
    PageHeader* m_header = nullptr;
    QStackedWidget* m_stack = nullptr;

    struct Entry {
        QString title;
        RailItem* item;
        PageBase* page;
    };
    std::vector<Entry> m_entries;
    int m_current = -1;
};
