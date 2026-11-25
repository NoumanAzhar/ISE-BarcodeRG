#pragma once

#include "PageBase.h"

#include "report.h"

namespace pos { class AppServices; struct DateRange; }

class DataTable;
class NeuSegmented;
class QDateEdit;
class QLabel;
class QListWidget;
class QWidget;

// Manager: the six reports as tables. Sales reports can be limited to a
// day, week, month, year or any period picked from the drop-down calendars.
class ReportsPage : public PageBase {
    Q_OBJECT
public:
    explicit ReportsPage(pos::AppServices& services, QWidget* parent = nullptr);

    void onActivated() override;       // data changes after every sale

private:
    QWidget* buildPicker();
    QWidget* buildViewer();

    void showReport(int index);
    void fillPeriod(int periodIndex);  // sets From / To from Day, Week, Month or Year
    void onRangeEdited();              // a date was picked: refresh the sales report
    void render(const pos::ReportTable& table);
    bool usesRange(int index) const;
    pos::DateRange range() const;

    pos::AppServices& m_services;
    int m_current = 0;

    QListWidget* m_list = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_about = nullptr;
    QWidget* m_periodBar = nullptr;
    NeuSegmented* m_period = nullptr;
    QDateEdit* m_from = nullptr;
    QDateEdit* m_to = nullptr;
    DataTable* m_table = nullptr;
    QLabel* m_footer = nullptr;
};
