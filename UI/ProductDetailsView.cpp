#include "ProductDetailsView.h"

#include "Format.h"
#include "Widgets.h"

#include <QGridLayout>
#include <QLabel>
#include <QStackedLayout>
#include <QVBoxLayout>

namespace {

// Picks the style-sheet look of a value label. "hint" + state "invalid" is the
// theme's danger colour, so nothing is hard-coded here.
void setRole(QLabel* label, const char* objectName, const char* state = nullptr)
{
    label->setObjectName(objectName);
    label->setProperty("state", state ? QVariant(state) : QVariant());
    repolish(label);
}

} // namespace

ProductDetailsView::ProductDetailsView(QWidget* parent)
    : QWidget(parent)
{
    m_stack = new QStackedLayout(this);
    m_stack->setContentsMargins(0, 0, 0, 0);

    m_message = makeLabel(QString(), "muted", this);
    m_message->setAlignment(Qt::AlignCenter);
    m_message->setWordWrap(true);

    m_stack->addWidget(buildDetailsPage());      // DetailsPage
    m_stack->addWidget(m_message);               // MessagePage
    m_stack->addWidget(new QWidget(this));       // BlankPage

    clear();
}

QWidget* ProductDetailsView::buildDetailsPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_name = makeLabel(QString(), "cardTitle", page);
    m_name->setWordWrap(true);

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(1, 1);

    m_acronym = addRow(grid, "Acronym");
    m_category = addRow(grid, "Category", &m_categoryKey);
    m_id = addRow(grid, "Product ID");
    m_barcode = addRow(grid, "Barcode");
    m_unit = addRow(grid, "Unit");
    m_price = addRow(grid, "Price");
    m_tax = addRow(grid, "Tax per unit");
    m_discount = addRow(grid, "Discount per unit");
    m_available = addRow(grid, "Available");

    layout->addWidget(m_name);
    layout->addLayout(grid);
    layout->addStretch(1);
    return page;
}

QLabel* ProductDetailsView::addRow(QGridLayout* grid, const QString& label, QLabel** keyOut)
{
    const int row = grid->rowCount();
    auto* key = makeLabel(label, "muted", this);
    auto* value = makeLabel(QString(), "fieldLabel", this);
    value->setWordWrap(true);
    grid->addWidget(key, row, 0, Qt::AlignTop);
    grid->addWidget(value, row, 1, Qt::AlignTop);
    if (keyOut)
        *keyOut = key;
    return value;
}

void ProductDetailsView::showProduct(const pos::Product& p, const std::optional<QString>& categoryName)
{
    m_name->setText(qs(p.name));
    m_acronym->setText(qs(p.acronym));

    m_categoryKey->setVisible(categoryName.has_value());
    m_category->setVisible(categoryName.has_value());
    if (categoryName.value_or(QString()).isEmpty()) {
        m_category->setText("none");
        setRole(m_category, "muted");
    } else {
        m_category->setText(*categoryName);
        setRole(m_category, "fieldLabel");
    }
    m_id->setText(qs(p.id));

    if (p.barcode.empty()) {
        m_barcode->setText("not assigned");
        setRole(m_barcode, "muted");
    } else {
        m_barcode->setText(qs(p.barcode));
        setRole(m_barcode, "mono");
    }

    m_unit->setText(unitName(p.unit));
    m_price->setText(formatPkr(p.unitPrice));
    m_tax->setText(formatPkr(p.tax));
    m_discount->setText(formatPkr(p.discount));

    if (p.availableQty <= 0) {
        m_available->setText("Out of stock");
        setRole(m_available, "hint", "invalid");
    } else {
        m_available->setText(formatQty(p.availableQty, p.unit));
        setRole(m_available, "fieldLabel");
    }

    m_stack->setCurrentIndex(DetailsPage);
}

void ProductDetailsView::showMessage(const QString& text)
{
    m_message->setText(text);
    m_stack->setCurrentIndex(MessagePage);
}

void ProductDetailsView::clear()
{
    m_message->clear();
    m_stack->setCurrentIndex(BlankPage);
}
