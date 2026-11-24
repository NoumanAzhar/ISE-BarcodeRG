#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTableWidget;
class QTableWidgetItem;

// A read-only table inside an inset well, used by every list screen
// (categories, inventory, discounts, orders, reports). Rows carry a key (an id)
// so a page never has to map row numbers back to data.
class DataTable : public QWidget {
    Q_OBJECT
public:
    enum class Align { Left, Right, Center };
    // Colour of a cell's text. The colours come from the theme, so they follow a theme switch.
    enum class Tone { Normal, Muted, Good, Warn, Bad };

    struct Column {
        QString title;
        Align align = Align::Left;
        bool stretch = false;      // takes the free width (give exactly one column this)
    };

    explicit DataTable(QWidget* parent = nullptr);

    void setColumns(const QList<Column>& columns);
    void setEmptyText(const QString& text);        // shown instead of the table when it has no rows

    void clearRows();                              // keeps columns; emits no signal
    int addRow(const QStringList& cells, const QString& key = QString());
    void addTotalsRow(const QStringList& cells);   // bold, cannot be selected
    void setTone(int row, int column, Tone tone);
    int rowCount() const;

    QString selectedKey() const;                   // empty when no row is selected
    void selectKey(const QString& key);            // empty or unknown clears; always emits selectionChanged
    void clearSelection();

signals:
    void selectionChanged(const QString& key);     // empty when the selection was cleared
    void activated(const QString& key);            // double click or Enter on a row

protected:
    void changeEvent(QEvent* event) override;      // theme switch: recolour the tones

private:
    QString keyAt(int row) const;
    void applyTone(QTableWidgetItem* item) const;
    void updateStack();

    QTableWidget* m_table = nullptr;
    QList<Align> m_aligns;                       // per column
    QLabel* m_empty = nullptr;
    QStackedWidget* m_stack = nullptr;
};
