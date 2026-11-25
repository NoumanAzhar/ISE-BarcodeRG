#pragma once

#include <QDialog>

#include "bill_pdf.h"

class Banner;

// The bill as a receipt on white paper, with a button that saves it as a PDF
// and opens it for printing. Enter, Escape and Done close it.
class BillDialog : public QDialog {
    Q_OBJECT
public:
    // `bill` must outlive the dialog (it is shown modally, so a local is fine).
    // `info` is the customer's phone number and the payment mode from the order;
    // an empty field is left off the bill.
    BillDialog(const pos::Bill& bill, const pos::BillOrderInfo& info,
               pos::IBillExporter& exporter, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void savePdf();

    const pos::Bill& m_bill;
    pos::IBillExporter& m_exporter;
    Banner* m_banner = nullptr;
};
