#pragma once

#include <QDialog>
#include <QString>

namespace pos { class AppServices; }

class DataTable;
class NeuButton;

// The orders that are on hold. Pick one (double click, Enter or Resume) to continue it.
class HoldListDialog : public QDialog {
    Q_OBJECT
public:
    explicit HoldListDialog(pos::AppServices& services, QWidget* parent = nullptr);

    QString chosenOrderId() const { return m_chosen; }

private:
    void choose(const QString& orderId);

    DataTable* m_table = nullptr;
    NeuButton* m_resumeBtn = nullptr;
    QString m_chosen;
};
