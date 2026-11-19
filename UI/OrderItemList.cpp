#include "OrderItemList.h"

#include "AppServices.h"
#include "Format.h"
#include "Neu.h"
#include "OrderTotals.h"
#include "Widgets.h"

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLayoutItem>
#include <QLocale>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>
#include <utility>

namespace {

constexpr int kPulseMs = 600;
constexpr double kWeightStep = 0.5;

bool isCount(const pos::Product& p) { return p.unit == pos::MeasureUnit::Count; }

// Plain number for the editor: "3" for counts, "1.5" for weights (no trailing zeros).
QString plainQty(double quantity, const pos::Product& p)
{
    if (isCount(p))
        return QString::number(quantity, 'f', 0);

    QString text = QString::number(quantity, 'f', 3);
    while (text.endsWith('0'))
        text.chop(1);
    if (text.endsWith('.'))
        text.chop(1);
    return text;
}

} // namespace

OrderItemList::OrderItemList(pos::AppServices& services, QWidget* parent)
    : QWidget(parent), m_services(services)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    NeuFrame* well = makeWell();
    auto* wellLayout = new QVBoxLayout(well);
    wellLayout->setContentsMargins(8, 8, 8, 8);

    m_empty = makeLabel("Scan a product to start", "muted", well);
    m_empty->setAlignment(Qt::AlignCenter);

    m_content = new QWidget;
    m_rowLayout = new QVBoxLayout(m_content);
    m_rowLayout->setContentsMargins(0, 0, 0, 0);
    m_rowLayout->setSpacing(6);
    m_rowLayout->addStretch(1);

    m_scroll = new QScrollArea(well);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->viewport()->setAutoFillBackground(false);
    m_scroll->setWidget(m_content);
    // QScrollArea::setWidget() switches autoFillBackground back on, which painted the rows' backdrop
    // in the palette colour (a black box in the light theme). Turn it off again, after setWidget().
    m_content->setAutoFillBackground(false);
    m_content->setObjectName("scrollContent");
    m_content->setStyleSheet("QWidget#scrollContent { background: transparent; }");

    wellLayout->addWidget(m_empty, 1);
    wellLayout->addWidget(m_scroll, 1);
    outer->addWidget(well);

    refresh();
}

QString OrderItemList::selectedProductId() const
{
    return m_selectedId;
}

void OrderItemList::clearSelection()
{
    select(QString());
}

bool OrderItemList::isOnRow(const QWidget* widget) const
{
    for (const Row& row : std::as_const(m_rowWidgets)) {
        if (row.frame == widget || row.frame->isAncestorOf(widget))
            return true;
    }
    return false;
}

// ------------------------------------------------------------------ rows

void OrderItemList::clearRows()
{
    for (const Row& row : std::as_const(m_rowWidgets)) {
        m_rowLayout->removeWidget(row.frame);
        row.frame->deleteLater();
    }
    m_rowWidgets.clear();
    m_products.clear();
}

void OrderItemList::refresh()
{
    const QString keep = m_selectedId;
    clearRows();

    const std::vector<pos::OrderItem>& items = m_services.order().current().items;
    for (const pos::OrderItem& item : items) {
        const std::optional<pos::Product> product = m_services.products().findById(item.productId);
        if (!product)
            continue;
        m_products.insert(qs(item.productId), *product);
        addRow(item, *product);
    }

    const bool empty = m_rowWidgets.isEmpty();
    m_empty->setVisible(empty);
    m_scroll->setVisible(!empty);

    if (m_rowWidgets.contains(keep)) {
        styleRow(keep);
    } else if (!keep.isEmpty()) {
        m_selectedId.clear();
        emit selectionChanged(QString());
    }
}

void OrderItemList::addRow(const pos::OrderItem& item, const pos::Product& product)
{
    const QString id = qs(item.productId);

    auto* frame = new NeuFrame(NeuStyle::Flat, 14, m_content);
    frame->setInteractive(true);
    frame->setMinimumHeight(64);

    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(14, 6, 10, 6);
    layout->setSpacing(8);

    // left: acronym over name
    auto* names = new QVBoxLayout;
    names->setSpacing(0);
    names->addWidget(makeLabel(qs(product.acronym), "fieldLabel", frame));
    names->addWidget(makeLabel(qs(product.name), "muted", frame));

    // middle: - [quantity] +
    auto* minus = new NeuButton("-", NeuButton::Variant::Normal, frame);
    auto* plus = new NeuButton("+", NeuButton::Variant::Normal, frame);
    auto* edit = new NeuLineEdit(frame);
    edit->setAlignment(Qt::AlignCenter);
    edit->setFixedWidth(88);
    if (isCount(product)) {
        edit->setValidator(new QIntValidator(0, 999999, edit));
    } else {
        auto* validator = new QDoubleValidator(0.0, 999999.0, 3, edit);
        validator->setLocale(QLocale::c());
        validator->setNotation(QDoubleValidator::StandardNotation);
        edit->setValidator(validator);
        edit->setLocale(QLocale::c());
    }

    // right: line total and remove
    auto* total = makeLabel(QString(), "fieldLabel", frame);
    total->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    total->setMinimumWidth(110);
    auto* removeBtn = new NeuButton(QString(kMultiply), NeuButton::Variant::Danger, frame);
    removeBtn->setToolTip("Remove from order");

    layout->addLayout(names, 1);
    layout->addWidget(minus);
    layout->addWidget(edit);
    layout->addWidget(plus);
    layout->addWidget(total);
    layout->addWidget(removeBtn);

    m_rowLayout->insertWidget(m_rowLayout->count() - 1, frame);   // before the stretch
    m_rowWidgets.insert(id, Row{ frame, edit, total });
    showQuantity(id, item.quantity);
    styleRow(id);

    connect(frame, &NeuFrame::clicked, this,
            [this, id]() { select(id == m_selectedId ? QString() : id); });   // click again = unselect
    connect(minus, &NeuButton::clicked, this, [this, id]() { step(id, -1); });
    connect(plus, &NeuButton::clicked, this, [this, id]() { step(id, +1); });
    connect(removeBtn, &NeuButton::clicked, this, [this, id]() { remove(id); });
    connect(edit, &QLineEdit::editingFinished, this, [this, id, edit]() {
        bool ok = false;
        const double q = QLocale::c().toDouble(edit->text(), &ok);
        commit(id, ok ? q : quantityOf(id));
    });
}

// -------------------------------------------------------------- selection

void OrderItemList::select(const QString& id)
{
    if (id == m_selectedId)
        return;
    const QString old = m_selectedId;
    m_selectedId = id;
    if (m_rowWidgets.contains(old))
        styleRow(old);
    styleRow(id);
    emit selectionChanged(id);
}

void OrderItemList::styleRow(const QString& id, bool pulsing)
{
    const Row row = m_rowWidgets.value(id);
    if (!row.frame)
        return;
    NeuOutline outline = NeuOutline::None;
    if (pulsing)
        outline = NeuOutline::Success;
    else if (id == m_selectedId)
        outline = NeuOutline::Accent;
    row.frame->setOutline(outline, Qt::SolidLine);
}

void OrderItemList::pulse(const QString& productId)
{
    const Row row = m_rowWidgets.value(productId);
    if (!row.frame)
        return;
    styleRow(productId, true);
    m_scroll->ensureWidgetVisible(row.frame);
    QTimer::singleShot(kPulseMs, this, [this, productId]() { styleRow(productId); });
}

// --------------------------------------------------------------- quantity

double OrderItemList::quantityOf(const QString& id) const
{
    const std::string key = ss(id);
    for (const pos::OrderItem& item : m_services.order().current().items) {
        if (item.productId == key)
            return item.quantity;
    }
    return 0.0;
}

void OrderItemList::showQuantity(const QString& id, double quantity)
{
    const Row row = m_rowWidgets.value(id);
    const pos::Product product = m_products.value(id);
    if (!row.frame)
        return;
    row.edit->setText(plainQty(quantity, product));
    row.total->setText(formatPkr(pos::lineTotal(product, quantity)));
}

void OrderItemList::step(const QString& id, int direction)
{
    const double size = isCount(m_products.value(id)) ? 1.0 : kWeightStep;
    commit(id, quantityOf(id) + direction * size);
}

void OrderItemList::commit(const QString& id, double quantity)
{
    const pos::Product product = m_products.value(id);
    const double current = quantityOf(id);

    const bool belowMinimum = isCount(product) ? quantity < 1.0 : quantity <= 0.0;
    if (belowMinimum || std::abs(quantity - current) < 1e-9) {
        showQuantity(id, current);               // nothing to change: restore the text
        return;
    }

    if (m_services.order().setQuantity(ss(id), quantity)) {
        showQuantity(id, quantity);
        emit changed();
        return;
    }

    showQuantity(id, current);
    const std::optional<pos::Product> fresh = m_services.products().findById(ss(id));
    const double max = fresh ? fresh->availableQty : product.availableQty;
    emit notice(QString("Not enough stock (max %1)").arg(plainQty(max, product)));
}

void OrderItemList::remove(const QString& id)
{
    if (!m_services.order().removeItem(ss(id)))
        return;
    refresh();                                   // also clears the selection if it was this row
    emit changed();
}
