#include "BillDialog.h"

#include "Bill.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"

#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

constexpr int kPaperWidth = 360;
constexpr double kMaxScreenShare = 0.8;

// Thin rule on the paper (white paper is the one allowed hard-coded colour area).
QFrame* makeRule(QWidget* parent)
{
    auto* rule = new QFrame(parent);
    rule->setFixedHeight(1);
    rule->setStyleSheet("background: #d9dee8;");
    return rule;
}

QLabel* makeBold(const QString& text, const char* objectName, QWidget* parent)
{
    QLabel* label = makeLabel(text, objectName, parent);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    return label;
}

// "key ........ value" on one line.
QHBoxLayout* makePair(const QString& key, QLabel* value, QWidget* parent)
{
    auto* row = new QHBoxLayout;
    row->addWidget(makeLabel(key, "paperMuted", parent));
    row->addStretch(1);
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(value);
    return row;
}

QHBoxLayout* makePair(const QString& key, const QString& value, QWidget* parent,
                      const char* valueObject = "paperInk")
{
    return makePair(key, makeLabel(value, valueObject, parent), parent);
}

QString negative(double amount)
{
    return amount > 0 ? "-" + formatPkr(amount) : formatPkr(amount);
}

} // namespace

BillDialog::BillDialog(const Bill& bill, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Bill");
    if (const QScreen* screen = QGuiApplication::primaryScreen())
        setMaximumHeight(int(screen->availableGeometry().height() * kMaxScreenShare));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // ------------------------------------------------------------- paper
    auto* paper = new QFrame(this);
    paper->setObjectName("paper");
    paper->setStyleSheet("QFrame#paper { background: #ffffff; border-radius: 14px; }");
    paper->setFixedWidth(kPaperWidth);
    auto* paperLayout = new QVBoxLayout(paper);
    paperLayout->setContentsMargins(20, 18, 20, 18);
    paperLayout->setSpacing(6);

    auto* title = new QHBoxLayout;
    title->addWidget(makeLabel("Bill", "paperTitle", paper));
    title->addStretch(1);
    auto* billId = makeLabel(qs(bill.getBillId()), "paperMuted", paper);
    title->addWidget(billId);
    paperLayout->addLayout(title);

    paperLayout->addLayout(makePair("Customer", qs(bill.getCustomerName()), paper));
    paperLayout->addLayout(makePair("Order", qs(bill.getOrderId()), paper));
    paperLayout->addLayout(makePair("Status", qs(bill.getOrderStatus()).toUpper(), paper));
    paperLayout->addSpacing(4);
    paperLayout->addWidget(makeRule(paper));

    // Items scroll inside the paper when the list is long.
    auto* items = new QWidget;
    auto* itemsLayout = new QVBoxLayout(items);
    itemsLayout->setContentsMargins(0, 4, 0, 4);
    itemsLayout->setSpacing(8);

    for (const auto& line : bill.getProducts()) {
        const pos::Product& product = line.first;
        const double qty = line.second;

        auto* names = new QVBoxLayout;
        names->setSpacing(0);
        names->addWidget(makeBold(qs(product.acronym), "paperInk", items));
        names->addWidget(makeLabel(formatQty(qty, product.unit) + " x " + formatPkr(product.unitPrice),
                                   "paperMuted", items));

        auto* amount = makeLabel(formatPkr(product.unitPrice * qty), "paperInk", items);
        amount->setAlignment(Qt::AlignRight | Qt::AlignTop);

        auto* row = new QHBoxLayout;
        row->addLayout(names, 1);
        row->addWidget(amount, 0, Qt::AlignTop);
        itemsLayout->addLayout(row);
    }
    itemsLayout->addStretch(1);

    auto* scroll = new QScrollArea(paper);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setWidget(items);
    // QScrollArea::setWidget() switches autoFillBackground back on (a black box on the white paper).
    // Turn it off again, after setWidget(), so the paper shows through.
    items->setAutoFillBackground(false);
    items->setObjectName("billItems");
    items->setStyleSheet("QWidget#billItems { background: transparent; }");
    paperLayout->addWidget(scroll, 1);

    paperLayout->addWidget(makeRule(paper));
    paperLayout->addSpacing(4);
    paperLayout->addLayout(makePair("Subtotal", formatPkr(bill.getTotalWithoutTax()), paper));
    paperLayout->addLayout(makePair("Tax", formatPkr(bill.getTotalTax()), paper));
    paperLayout->addLayout(makePair("Discount", negative(bill.getTotalDiscount()), paper));
    paperLayout->addSpacing(4);

    auto* payable = new QHBoxLayout;
    payable->addWidget(makeBold("Payable", "paperInk", paper));
    payable->addStretch(1);
    payable->addWidget(makeLabel(formatPkr(bill.getTotalBill()), "paperTotal", paper));
    paperLayout->addLayout(payable);

    // -------------------------------------------------------------- Done
    auto* doneBtn = new NeuButton("Done", NeuButton::Variant::Primary, this);
    doneBtn->setDefault(true);
    connect(doneBtn, &NeuButton::clicked, this, &QDialog::accept);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(doneBtn);

    layout->addWidget(paper, 1);
    layout->addLayout(buttons);
}

void BillDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        accept();
        return;
    default:
        QDialog::keyPressEvent(event);
    }
}
