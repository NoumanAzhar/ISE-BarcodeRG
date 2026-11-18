#include "OrderPage.h"

#include "AppServices.h"
#include "CameraScannerWidget.h"
#include "CheckoutFlow.h"
#include "Format.h"
#include "Neu.h"
#include "OrderItemList.h"
#include "OrderTotals.h"
#include "ProductDialog.h"
#include "Widgets.h"
#include "order.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QApplication>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QShortcut>
#include <QVBoxLayout>

namespace {

// Optional field: empty is fine. Once the user has typed something it must be
// exactly 11 digits starting with "03".
bool isValidContact(const QString& contact)
{
    if (contact.size() != 11 || !contact.startsWith(QLatin1String("03")))
        return false;
    for (const QChar c : contact)
        if (!c.isDigit())
            return false;
    return true;
}

// True if the widget (or a parent below `stop`) is a button, text input or scroll bar:
// clicks there must not count as "empty space".
bool isOnControl(const QWidget* widget, const QWidget* stop)
{
    for (const QWidget* w = widget; w && w != stop; w = w->parentWidget()) {
        if (qobject_cast<const QAbstractButton*>(w) || qobject_cast<const QLineEdit*>(w)
            || qobject_cast<const QAbstractSlider*>(w))
            return true;
    }
    return false;
}

} // namespace

namespace {

constexpr int kScanCooldownMs = 1500;
constexpr int kLongMessageMs = 4000;

QString plainNumber(double value)
{
    return QString::number(value, 'g', 6);
}

} // namespace

OrderPage::OrderPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent), m_services(services)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room
    root->addWidget(buildScanCard(), 2);
    root->addWidget(buildOrderCard(), 3);

    for (const QKeySequence& keys : { QKeySequence(Qt::CTRL | Qt::Key_Return),
                                      QKeySequence(Qt::CTRL | Qt::Key_Enter) }) {
        auto* shortcut = new QShortcut(keys, this);
        connect(shortcut, &QShortcut::activated, this, &OrderPage::generateBill);
    }

    refreshAll();
}

// ---------------------------------------------------------------- building

QWidget* OrderPage::buildScanCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    m_camera = new CameraScannerWidget(card);
    m_banner = new Banner(card);

    // Fallback when the webcam does not work.
    m_code = new NeuLineEdit(card);
    m_code->setPlaceholderText("Type barcode");
    m_code->setMaxLength(6);
    m_code->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), m_code));
    auto* addBtn = new NeuButton("Add", NeuButton::Variant::Normal, card);

    auto* typed = new QHBoxLayout;
    typed->setSpacing(2);
    typed->addWidget(m_code, 1);
    typed->addWidget(addBtn);

    layout->addWidget(m_camera, 1);
    layout->addWidget(m_banner);
    layout->addLayout(typed);

    connect(m_camera, &CameraScannerWidget::barcodeScanned, this, &OrderPage::onCameraScanned);
    connect(m_camera, &CameraScannerWidget::cameraFailed, this,
            [this](const QString& message) { m_banner->showMessage(message, Banner::Error, kLongMessageMs); });
    connect(m_code, &NeuLineEdit::returnPressed, this, [this]() { addBarcode(m_code->text()); });
    connect(addBtn, &NeuButton::clicked, this, [this]() { addBarcode(m_code->text()); });
    return card;
}

QWidget* OrderPage::buildOrderCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    // customer
    m_name = new NeuLineEdit(card);
    m_name->setPlaceholderText("Customer name");
    m_contact = new NeuLineEdit(card);
    m_contact->setPlaceholderText("Contact (03XXXXXXXXX)");
    m_contact->setMaxLength(11);
    m_contact->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), m_contact));

    connect(m_name, &QLineEdit::textChanged, this, [this]() {
        if (!m_name->text().trimmed().isEmpty())
            m_name->setState("");
    });
    connect(m_contact, &QLineEdit::textChanged, this, [this]() {
        m_contact->setState("");
    });

    auto* customer = new QHBoxLayout;
    customer->setSpacing(2);
    customer->addWidget(makeLabel("Customer", "fieldLabel", card));
    customer->addWidget(m_name, 1);
    customer->addWidget(m_contact, 1);

    m_list = new OrderItemList(m_services, card);

    // totals
    m_subtotal = makeLabel(QString(), "mono", card);
    m_tax = makeLabel(QString(), "mono", card);
    m_discount = makeLabel(QString(), "mono", card);
    auto* sums = new QGridLayout;
    sums->setHorizontalSpacing(16);
    sums->setVerticalSpacing(2);
    const QPair<const char*, QLabel*> sumRows[] = {
        { "Subtotal", m_subtotal }, { "Tax", m_tax }, { "Discount", m_discount } };
    int row = 0;
    for (const auto& r : sumRows) {
        sums->addWidget(makeLabel(r.first, "muted", card), row, 0);
        r.second->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sums->addWidget(r.second, row, 1);
        ++row;
    }

    m_payable = makeLabel(QString(), "totalValue", card);
    m_payable->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* payable = new QVBoxLayout;
    payable->setSpacing(0);
    payable->addWidget(makeLabel("PAYABLE", "sectionTitle", card), 0, Qt::AlignRight);
    payable->addWidget(m_payable);

    auto* totals = new QHBoxLayout;
    totals->addLayout(sums);
    totals->addStretch(1);
    totals->addLayout(payable);

    // actions
    m_descriptionBtn = new NeuButton("Description", NeuButton::Variant::Normal, card);
    m_descriptionBtn->setEnabled(false);
    auto* clearBtn = new NeuButton("Clear", NeuButton::Variant::Normal, card);
    auto* billBtn = new NeuButton("Generate Bill", NeuButton::Variant::Primary, card);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(2);
    actions->addWidget(m_descriptionBtn);
    actions->addWidget(clearBtn);
    actions->addStretch(1);
    actions->addWidget(billBtn);

    layout->addLayout(customer);
    layout->addWidget(m_list, 1);
    layout->addLayout(totals);
    layout->addLayout(actions);

    connect(m_name, &NeuLineEdit::textChanged, this, [this]() { m_name->setState(QString()); });
    connect(m_list, &OrderItemList::changed, this, &OrderPage::updateTotals);
    connect(m_list, &OrderItemList::selectionChanged, this,
            [this](const QString& id) { m_descriptionBtn->setEnabled(!id.isEmpty()); });
    connect(m_list, &OrderItemList::notice, this,
            [this](const QString& text) { m_banner->showMessage(text, Banner::Warning); });
    connect(m_descriptionBtn, &NeuButton::clicked, this, &OrderPage::showDescription);
    connect(clearBtn, &NeuButton::clicked, this, &OrderPage::clearItems);
    connect(billBtn, &NeuButton::clicked, this, &OrderPage::generateBill);
    return card;
}

// ------------------------------------------------------------- page hooks

void OrderPage::onActivated()
{
    m_active = true;
    qApp->installEventFilter(this);      // to unselect a row when empty space is clicked
    refreshAll();
}

void OrderPage::onDeactivated()
{
    m_active = false;
    qApp->removeEventFilter(this);
    m_camera->setActive(false);
}

void OrderPage::resetState()
{
    resetFields();                       // customer, contact and typed barcode
    m_list->clearSelection();
    m_banner->clearMessage();
    m_lastScan.clear();
    refreshAll();
}

bool OrderPage::eventFilter(QObject* watched, QEvent* event)
{
    if (m_active && event->type() == QEvent::MouseButtonPress && watched->isWidgetType()
        && !m_list->selectedProductId().isEmpty()) {
        const auto* widget = static_cast<const QWidget*>(watched);
        const auto* mouse = static_cast<const QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton
            && widget->window() == window()              // not inside a dialog
            && (widget == this || isAncestorOf(widget))
            && !m_list->isOnRow(widget)                  // rows toggle themselves
            && !isOnControl(widget, this))
            m_list->clearSelection();
    }
    return PageBase::eventFilter(watched, event);
}

bool OrderPage::canLeave()
{
    if (m_services.order().current().items.empty())
        return true;

    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle("Unfinished order");
    box.setText("This order has items that are not billed yet.");
    QPushButton* keep = box.addButton("Keep order", QMessageBox::RejectRole);
    QPushButton* discard = box.addButton("Discard order", QMessageBox::DestructiveRole);
    box.setDefaultButton(keep);
    box.exec();

    if (box.clickedButton() != discard)
        return false;

    m_services.order().clear();
    resetFields();
    refreshAll();
    return true;
}

// ---------------------------------------------------------------- scanning

void OrderPage::onCameraScanned(const QString& digits)
{
    const bool repeat = digits == m_lastScan && m_lastScanTime.isValid()
                        && m_lastScanTime.elapsed() < kScanCooldownMs;
    m_lastScan = digits;
    m_lastScanTime.restart();
    if (!repeat)
        addBarcode(digits);
}

void OrderPage::addBarcode(const QString& code)
{
    const QString barcode = code.trimmed();
    if (barcode.isEmpty())
        return;

    const pos::AddResult result = m_services.order().addByBarcode(ss(barcode));
    const QString name = qs(result.product.name);

    switch (result.status) {
    case pos::AddStatus::Added:
        m_banner->showMessage(QString("%1 x%2").arg(name, plainNumber(result.quantityInOrder)),
                              Banner::Success);
        m_code->clear();
        m_list->refresh();
        m_list->pulse(qs(result.product.id));
        updateTotals();
        break;
    case pos::AddStatus::UnknownBarcode:
        m_banner->showMessage("No product has this barcode", Banner::Error);
        break;
    case pos::AddStatus::InsufficientStock:
        m_banner->showMessage("Not enough stock for " + name, Banner::Error);
        break;
    case pos::AddStatus::InvalidQuantity:
        m_banner->showMessage("Invalid quantity", Banner::Error);
        break;
    }
}

// ----------------------------------------------------------------- actions

void OrderPage::showDescription()
{
    const std::optional<pos::Product> product =
        m_services.products().findById(ss(m_list->selectedProductId()));
    if (!product)
        return;
    ProductDialog dialog(*product, this);
    dialog.exec();
}

void OrderPage::clearItems()
{
    m_services.order().clear();
    refreshAll();
}

void OrderPage::generateBill()
{
    if (!isVisible())
        return;

    if (m_services.order().current().items.empty()) {
        m_banner->showMessage("The order has no items", Banner::Error);
        return;
    }

    const QString name = m_name->text().trimmed();
    if (name.isEmpty()) {
        m_name->setState("invalid");
        m_name->setFocus();
        m_banner->showMessage("Enter the customer name", Banner::Error);
        return;
    }

    const QString contact = m_contact->text().trimmed();
    if (!contact.isEmpty() && !isValidContact(contact)) {   // contact is optional
        m_contact->setState("invalid");
        m_contact->setFocus();
        m_banner->showMessage("Enter a valid number", Banner::Error);
        return;
    }

	const bool cameraWasOn = m_camera->isActive();
    m_camera->setActive(false);
    const CheckoutFlow::Outcome outcome =
        CheckoutFlow(m_services).run(name, contact, this);
    if (m_active && cameraWasOn)
        m_camera->setActive(true);

    switch (outcome.result) {
    case CheckoutFlow::Result::Cancelled:
        break;
    case CheckoutFlow::Result::Saved:
        resetFields();
        refreshAll();
        m_banner->showMessage(outcome.message, Banner::Success, kLongMessageMs);
        break;
    case CheckoutFlow::Result::SavedWithWarning:
        resetFields();
        refreshAll();
        m_banner->showMessage(outcome.message, Banner::Warning, kLongMessageMs);
        break;
    case CheckoutFlow::Result::Failed:
        m_banner->showMessage(outcome.message, Banner::Error, kLongMessageMs);
        break;
    }
}

// ------------------------------------------------------------------ screen

void OrderPage::resetFields()
{
    m_name->clear();
    m_name->setState("");
    m_contact->clear();
    m_contact->setState("");
    m_code->clear();
    m_code->setState("");
}

void OrderPage::refreshAll()
{
    m_list->refresh();
    updateTotals();
}

void OrderPage::updateTotals()
{
    const pos::OrderTotals totals =
        pos::computeTotals(m_services.order().current(), m_services.products());
    m_subtotal->setText(formatPkr(totals.withoutTax));
    m_tax->setText(formatPkr(totals.tax));
    m_discount->setText(formatPkr(totals.discount));
    m_payable->setText(formatPkr(totals.payable));
}
