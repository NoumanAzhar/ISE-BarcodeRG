#pragma once

#include <QDialog>

class Banner;
namespace pos { class Bill; class IBillExporter; }

// The bill as a receipt on white paper, with a button that saves it as a PDF
// and opens it for printing. Enter, Escape and Done close it.
class BillDialog : public QDialog {
    Q_OBJECT
public:
    // `bill` must outlive the dialog (it is shown modally, so a local is fine).
    BillDialog(const pos::Bill& bill, pos::IBillExporter& exporter, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void savePdf();

    const pos::Bill& m_bill;
    pos::IBillExporter& m_exporter;
    Banner* m_banner = nullptr;
};
