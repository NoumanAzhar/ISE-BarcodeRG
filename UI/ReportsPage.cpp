#include "ReportsPage.h"

#include "AppServices.h"
#include "DataTable.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"
#include "date_utils.h"

#include <QDate>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>
#include <iterator>

namespace {

struct ReportInfo {
    const char* name;          // shown in the list
    const char* about;         // one line under the title
    bool usesRange;            // sales reports take a period
};

enum Report { Incomplete, Completed, CashSales, AllSales, BestSellers, MostInStock };

const ReportInfo kReports[] = {
    { "Incomplete orders", "Orders that are not paid or archived yet.", false },
    { "Completed orders", "Paid and archived orders.", false },
    { "Cash sales", "Payments made in cash during the period.", true },
    { "All sales", "Every sale in the period, with its discounts.", true },
    { "Best sellers", "The items sold the most, with what is left on the shelf.", false },
    { "Most in stock", "The items with the most stock left: no need to buy these yet.", false },
};

enum PeriodIndex { PDay, PWeek, PMonth, PYear };

QString todayIso()
{
    return QDate::currentDate().toString(Qt::ISODate);
}

// "1,250.00" or "10" or "15%" -> true; used to right-align number columns.
bool looksNumeric(const QString& text)
{
    QString t = text;
    t.remove(',');
    t.remove('%');
    if (t.startsWith("PKR "))
        t = t.mid(4);
    bool ok = false;
    t.toDouble(&ok);
    return ok;
}

} // namespace

ReportsPage::ReportsPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent), m_services(services)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room
    root->addWidget(buildPicker());
    root->addWidget(buildViewer(), 1);

    fillPeriod(PDay);
    m_list->setCurrentRow(Incomplete);
}

// ---------------------------------------------------------------- building

QWidget* ReportsPage::buildPicker()
{
    NeuFrame* card = makeCard();
    card->setFixedWidth(270);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 18, 16, 16);
    layout->setSpacing(8);

    NeuFrame* well = makeWell();
    auto* wellLayout = new QVBoxLayout(well);
    wellLayout->setContentsMargins(6, 6, 6, 6);

    m_list = new QListWidget(well);
    m_list->setFocusPolicy(Qt::StrongFocus);
    for (const ReportInfo& info : kReports)
        m_list->addItem(info.name);
    wellLayout->addWidget(m_list);

    layout->addWidget(makeLabel("REPORTS", "sectionTitle", card));
    layout->addWidget(well, 1);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0)
            showReport(row);
    });
    return card;
}

QWidget* ReportsPage::buildViewer()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(4);

    m_title = makeLabel(QString(), "cardTitle", card);
    m_about = makeLabel(QString(), "muted", card);
    m_about->setWordWrap(true);

    // ---- period (sales reports only)
    m_periodBar = new QWidget(card);
    auto* bar = new QHBoxLayout(m_periodBar);
    bar->setContentsMargins(0, 0, 0, 0);
    bar->setSpacing(2);

    m_period = new NeuSegmented(QStringList{ "Day", "Week", "Month", "Year" }, m_periodBar);
    m_period->setMinimumWidth(340);
    m_from = new NeuLineEdit(m_periodBar);
    m_from->setPlaceholderText("From");
    m_to = new NeuLineEdit(m_periodBar);
    m_to->setPlaceholderText("To");
    for (NeuLineEdit* edit : { m_from, m_to }) {
        edit->setMaxLength(10);
        edit->setMinimumWidth(150);
        edit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9-]*"), edit));
        connect(edit, &QLineEdit::textEdited, this, &ReportsPage::onRangeEdited);
    }
    bar->addWidget(m_period);
    bar->addStretch(1);
    bar->addWidget(m_from);
    bar->addWidget(m_to);

    m_table = new DataTable(card);
    m_footer = makeLabel(QString(), "muted", card);
    m_footer->setWordWrap(true);

    layout->addWidget(m_title);
    layout->addWidget(m_about);
    layout->addWidget(m_periodBar);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_footer);

    connect(m_period, &NeuSegmented::currentChanged, this, &ReportsPage::fillPeriod);
    return card;
}

// ------------------------------------------------------------- page hooks

void ReportsPage::onActivated()
{
    showReport(m_current);
}

// ----------------------------------------------------------------- periods

bool ReportsPage::usesRange(int index) const
{
    return index >= 0 && index < int(std::size(kReports)) && kReports[index].usesRange;
}

pos::DateRange ReportsPage::range() const
{
    return pos::DateRange{ ss(m_from->text().trimmed()), ss(m_to->text().trimmed()) };
}

void ReportsPage::fillPeriod(int periodIndex)
{
    pos::Period period = pos::Period::Day;
    switch (periodIndex) {
    case PWeek:  period = pos::Period::Week;  break;
    case PMonth: period = pos::Period::Month; break;
    case PYear:  period = pos::Period::Year;  break;
    default:     break;
    }
    const pos::DateRange r = pos::rangeFor(period, ss(todayIso()));
    m_from->setText(qs(r.from));
    m_to->setText(qs(r.to));
    m_from->setState(QString());
    m_to->setState(QString());
    if (usesRange(m_current))
        showReport(m_current);
}

void ReportsPage::onRangeEdited()
{
    const pos::DateRange r = range();
    const bool fromOk = pos::isValidDate(r.from);
    const bool toOk = pos::isValidDate(r.to);
    m_from->setState(m_from->text().length() == 10 ? (fromOk ? "valid" : "invalid") : "");
    m_to->setState(m_to->text().length() == 10 ? (toOk ? "valid" : "invalid") : "");
    if (fromOk && toOk && r.from <= r.to && usesRange(m_current))
        showReport(m_current);
}

// ----------------------------------------------------------------- showing

void ReportsPage::showReport(int index)
{
    m_current = index;
    {
        const QSignalBlocker blocker(m_list);
        m_list->setCurrentRow(index);
    }
    m_periodBar->setVisible(usesRange(index));

    pos::ReportTable table;
    switch (index) {
    case Incomplete:  table = m_services.reports().incompleteOrders(); break;
    case Completed:   table = m_services.reports().completedOrders(); break;
    case CashSales:   table = m_services.reports().cashSales(range()); break;
    case AllSales:    table = m_services.reports().allSales(range()); break;
    case BestSellers: table = m_services.reports().topSold(); break;
    default:          table = m_services.reports().topRemaining(); break;
    }

    m_title->setText(table.title.empty() ? QString(kReports[index].name) : qs(table.title));
    m_about->setText(kReports[index].about);
    render(table);
}

void ReportsPage::render(const pos::ReportTable& table)
{
    const int columnCount = int(table.columns.size());

    // Number columns are right-aligned; the widest text column takes the free width.
    QList<DataTable::Column> columns;
    int stretch = 0;
    double widest = -1;
    for (int c = 0; c < columnCount; ++c) {
        bool numeric = !table.rows.empty();
        double totalLength = 0;
        for (const auto& row : table.rows) {
            const QString cell = c < int(row.size()) ? qs(row[c]) : QString();
            totalLength += cell.length();
            if (!cell.isEmpty() && !looksNumeric(cell))
                numeric = false;
        }
        const double average = table.rows.empty() ? 0 : totalLength / table.rows.size();
        if (!numeric && average > widest) {
            widest = average;
            stretch = c;
        }
        columns.append({ qs(table.columns[c]), numeric ? DataTable::Align::Right : DataTable::Align::Left, false });
    }
    if (!columns.isEmpty())
        columns[stretch].stretch = true;

    m_table->setColumns(columns);
    m_table->setEmptyText("Nothing to show for this report.");
    m_table->clearRows();
    for (const auto& row : table.rows) {
        QStringList cells;
        for (int c = 0; c < columnCount; ++c)
            cells << (c < int(row.size()) ? qs(row[c]) : QString());
        m_table->addRow(cells);
    }

    m_footer->clear();
    if (!table.totals.empty()) {
        QStringList totals;
        for (const std::string& t : table.totals)
            totals << qs(t);
        if (int(totals.size()) == columnCount && !table.rows.empty()) {
            m_table->addTotalsRow(totals);
        } else {
            totals.removeAll(QString());
            m_footer->setText(totals.join(QStringLiteral("   ") + QString(kMiddleDot) + QStringLiteral("   ")));
        }
    }
}
