#include "DataTable.h"

#include "Theme.h"
#include "Widgets.h"

#include <QEvent>
#include <QFont>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr int kRowHeight = 42;
constexpr int kToneRole = Qt::UserRole + 1;

Qt::Alignment flagsFor(DataTable::Align align)
{
    switch (align) {
    case DataTable::Align::Right:  return Qt::AlignRight | Qt::AlignVCenter;
    case DataTable::Align::Center: return Qt::AlignHCenter | Qt::AlignVCenter;
    default:                       return Qt::AlignLeft | Qt::AlignVCenter;
    }
}

} // namespace

DataTable::DataTable(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    NeuFrame* well = makeWell();
    auto* wellLayout = new QVBoxLayout(well);
    wellLayout->setContentsMargins(8, 8, 8, 8);

    m_table = new QTableWidget(0, 0, well);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setTextElideMode(Qt::ElideRight);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(kRowHeight);
    m_table->horizontalHeader()->setHighlightSections(false);

    m_empty = makeLabel(QString(), "muted", well);
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setWordWrap(true);

    m_stack = new QStackedWidget(well);
    m_stack->addWidget(m_table);                 // 0
    m_stack->addWidget(m_empty);                 // 1
    wellLayout->addWidget(m_stack);
    outer->addWidget(well);

    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            [this]() { emit selectionChanged(selectedKey()); });
    connect(m_table, &QTableWidget::itemActivated, this, [this](QTableWidgetItem* item) {
        const QString key = keyAt(item->row());
        if (!key.isEmpty())
            emit activated(key);
    });
    updateStack();
}

void DataTable::setColumns(const QList<Column>& columns)
{
    m_table->setColumnCount(int(columns.size()));
    m_aligns.clear();
    QHeaderView* header = m_table->horizontalHeader();
    for (int i = 0; i < columns.size(); ++i) {
        auto* item = new QTableWidgetItem(columns[i].title);
        item->setTextAlignment(flagsFor(columns[i].align));
        m_aligns.append(columns[i].align);
        m_table->setHorizontalHeaderItem(i, item);
        header->setSectionResizeMode(i, columns[i].stretch ? QHeaderView::Stretch
                                                           : QHeaderView::ResizeToContents);
    }
}

void DataTable::setEmptyText(const QString& text)
{
    m_empty->setText(text);
}

void DataTable::clearRows()
{
    const bool wasBlocked = m_table->blockSignals(true);
    m_table->setRowCount(0);
    m_table->blockSignals(wasBlocked);
    updateStack();
}

int DataTable::addRow(const QStringList& cells, const QString& key)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    for (int col = 0; col < m_table->columnCount(); ++col) {
        auto* item = new QTableWidgetItem(col < cells.size() ? cells[col] : QString());
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setTextAlignment(flagsFor(m_aligns.value(col, Align::Left)));
        if (col == 0)
            item->setData(Qt::UserRole, key);
        m_table->setItem(row, col, item);
    }
    updateStack();
    return row;
}

void DataTable::addTotalsRow(const QStringList& cells)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    for (int col = 0; col < m_table->columnCount(); ++col) {
        auto* item = new QTableWidgetItem(col < cells.size() ? cells[col] : QString());
        item->setFlags(Qt::ItemIsEnabled);      // not selectable
        item->setTextAlignment(flagsFor(m_aligns.value(col, Align::Left)));
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        m_table->setItem(row, col, item);
    }
    updateStack();
}

void DataTable::setTone(int row, int column, Tone tone)
{
    QTableWidgetItem* item = m_table->item(row, column);
    if (!item)
        return;
    item->setData(kToneRole, int(tone));
    applyTone(item);
}

int DataTable::rowCount() const
{
    return m_table->rowCount();
}

QString DataTable::keyAt(int row) const
{
    const QTableWidgetItem* item = m_table->item(row, 0);
    return item ? item->data(Qt::UserRole).toString() : QString();
}

QString DataTable::selectedKey() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? QString() : keyAt(rows.first().row());
}

void DataTable::selectKey(const QString& key)
{
    const bool wasBlocked = m_table->blockSignals(true);
    m_table->clearSelection();
    for (int row = 0; row < m_table->rowCount() && !key.isEmpty(); ++row) {
        if (keyAt(row) == key) {
            m_table->selectRow(row);
            break;
        }
    }
    m_table->blockSignals(wasBlocked);
    emit selectionChanged(selectedKey());
}

void DataTable::clearSelection()
{
    m_table->clearSelection();                   // emits selectionChanged through the table
}

// ----------------------------------------------------------------- colours

void DataTable::applyTone(QTableWidgetItem* item) const
{
    const Theme::Colors& c = Theme::colors();
    const auto tone = static_cast<Tone>(item->data(kToneRole).toInt());

    QFont font = item->font();
    font.setBold(tone == Tone::Good || tone == Tone::Warn || tone == Tone::Bad);
    item->setFont(font);

    switch (tone) {
    case Tone::Good:   item->setForeground(c.success); break;
    case Tone::Warn:   item->setForeground(c.warning); break;
    case Tone::Bad:    item->setForeground(c.danger);  break;
    case Tone::Muted:  item->setForeground(c.muted);   break;
    case Tone::Normal: item->setData(Qt::ForegroundRole, QVariant()); break;
    }
}

void DataTable::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() != QEvent::StyleChange || !m_table)
        return;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        for (int col = 0; col < m_table->columnCount(); ++col) {
            if (QTableWidgetItem* item = m_table->item(row, col); item && item->data(kToneRole).isValid())
                applyTone(item);
        }
    }
}

void DataTable::updateStack()
{
    m_stack->setCurrentIndex(m_table->rowCount() == 0 ? 1 : 0);
}
