#include "InventoryPage.h"

#include "AppServices.h"
#include "DataTable.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {

enum Filter { ShowAll, ShowOutOfStock };

bool matches(const pos::InventoryRow& row, const QString& needle)
{
    return qs(row.name).toLower().contains(needle)
        || qs(row.acronym).toLower().contains(needle)
        || qs(row.categoryName).toLower().contains(needle);
}

} // namespace

InventoryPage::InventoryPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent), m_services(services)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room
    root->addWidget(buildListSide(), 3);
    root->addWidget(buildEditor(), 2);

    showSelected();
}

// ---------------------------------------------------------------- building

QWidget* InventoryPage::buildListSide()
{
    auto* side = new QWidget(this);
    auto* layout = new QVBoxLayout(side);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_search = new NeuLineEdit(side);
    m_search->setPlaceholderText("Search product or category");
    m_filter = new NeuSegmented(QStringList{ "All", "Out of stock" }, side);
    m_filter->setMinimumWidth(260);

    auto* top = new QHBoxLayout;
    top->setSpacing(2);
    top->addWidget(m_search, 1);
    top->addWidget(m_filter);

    m_table = new DataTable(side);
    m_table->setColumns({
        { "Acronym", DataTable::Align::Left, false },
        { "Product", DataTable::Align::Left, true },
        { "Category", DataTable::Align::Left, false },
        { "In stock", DataTable::Align::Right, false },
    });

    layout->addLayout(top);
    layout->addWidget(m_table, 1);

    connect(m_search, &QLineEdit::textChanged, this, &InventoryPage::applyFilter);
    connect(m_filter, &NeuSegmented::currentChanged, this, &InventoryPage::applyFilter);
    connect(m_table, &DataTable::selectionChanged, this, &InventoryPage::showSelected);
    return side;
}

QWidget* InventoryPage::buildEditor()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(6);

    m_placeholder = makeLabel("Select a product to update its stock.", "muted", card);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);

    // ---- shown once a product is selected
    m_editor = new QWidget(card);
    auto* editor = new QVBoxLayout(m_editor);
    editor->setContentsMargins(0, 0, 0, 0);
    editor->setSpacing(4);

    m_name = makeLabel(QString(), "cardTitle", m_editor);
    m_name->setWordWrap(true);
    m_sub = makeLabel(QString(), "muted", m_editor);
    m_stock = makeLabel(QString(), "totalValue", m_editor);
    m_stockNote = makeLabel("Out of stock", "hint", m_editor);
    m_stockNote->setProperty("state", "invalid");

    m_amount = new NeuLineEdit(m_editor);
    m_amount->setPlaceholderText("Amount");
    m_amount->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[0-9]{0,7}([.][0-9]{0,3})?"), m_amount));

    auto* hint = makeLabel("Add stock adds the amount to what is on the shelf. "
                           "Set quantity replaces the stock, for example after counting it.",
                           "muted", m_editor);
    hint->setWordWrap(true);

    m_banner = new Banner(m_editor);

    m_setBtn = new NeuButton("Set quantity", NeuButton::Variant::Normal, m_editor);
    m_addBtn = new NeuButton("Add stock", NeuButton::Variant::Primary, m_editor);
    m_addBtn->setDefault(true);
    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    buttons->addWidget(m_setBtn);
    buttons->addStretch(1);
    buttons->addWidget(m_addBtn);

    editor->addWidget(makeLabel("PRODUCT", "sectionTitle", m_editor));
    editor->addWidget(m_name);
    editor->addWidget(m_sub);
    editor->addSpacing(10);
    editor->addWidget(makeLabel("IN STOCK", "sectionTitle", m_editor));
    editor->addWidget(m_stock);
    editor->addWidget(m_stockNote);
    editor->addSpacing(10);
    editor->addWidget(makeField("Amount", m_amount, m_editor));
    editor->addWidget(hint);
    editor->addStretch(1);
    editor->addWidget(m_banner);
    editor->addLayout(buttons);

    layout->addWidget(m_placeholder, 1);
    layout->addWidget(m_editor, 1);

    connect(m_amount, &QLineEdit::textChanged, this, [this]() { m_amount->setState(QString()); });
    connect(m_amount, &QLineEdit::returnPressed, this, [this]() { apply(true); });
    connect(m_addBtn, &NeuButton::clicked, this, [this]() { apply(true); });
    connect(m_setBtn, &NeuButton::clicked, this, [this]() { apply(false); });
    return card;
}

// ------------------------------------------------------------- page hooks

void InventoryPage::onActivated()
{
    reload();
}

void InventoryPage::resetState()
{
    m_search->clear();
    m_filter->setCurrentIndex(ShowAll);
    m_table->clearSelection();
    m_banner->clearMessage();
}

// -------------------------------------------------------------------- list

void InventoryPage::reload()
{
    m_rows = m_services.inventory().list();      // sorted by name
    applyFilter();
}

void InventoryPage::applyFilter()
{
    const QString keep = m_table->selectedKey();
    const QString needle = m_search->text().trimmed().toLower();
    const bool onlyOut = m_filter->currentIndex() == ShowOutOfStock;

    m_table->setEmptyText(onlyOut && needle.isEmpty() ? "Nothing is out of stock."
                          : m_rows.empty()            ? "There are no products yet."
                                                      : "No product matches.");
    m_table->clearRows();
    for (const pos::InventoryRow& row : m_rows) {
        if (onlyOut && row.quantity > 0)
            continue;
        if (!needle.isEmpty() && !matches(row, needle))
            continue;
        const int at = m_table->addRow({ qs(row.acronym), qs(row.name), qs(row.categoryName),
                                         formatQty(row.quantity, row.unit) },
                                       qs(row.productId));
        if (row.quantity <= 0)
            m_table->setTone(at, 3, DataTable::Tone::Bad);
        if (row.categoryName.empty())
            m_table->setTone(at, 2, DataTable::Tone::Muted);
    }
    m_table->selectKey(keep);                    // refreshes the editor
}

const pos::InventoryRow* InventoryPage::selectedRow() const
{
    const std::string id = ss(m_table->selectedKey());
    const auto it = std::find_if(m_rows.begin(), m_rows.end(),
                                 [&id](const pos::InventoryRow& r) { return r.productId == id; });
    return (id.empty() || it == m_rows.end()) ? nullptr : &*it;
}

// ------------------------------------------------------------------ editor

void InventoryPage::showSelected()
{
    const pos::InventoryRow* row = selectedRow();
    m_placeholder->setVisible(row == nullptr);
    m_editor->setVisible(row != nullptr);
    if (!row)
        return;

    m_name->setText(qs(row->name));
    const QString category = row->categoryName.empty() ? QStringLiteral("No category")
                                                       : qs(row->categoryName);
    const QString dot = QStringLiteral("  ") + QString(kMiddleDot) + QStringLiteral("  ");
    m_sub->setText(qs(row->acronym) + dot + QStringLiteral("sold by ") + unitName(row->unit) + dot + category);
    m_stock->setText(formatQty(row->quantity, row->unit));
    m_stockNote->setVisible(row->quantity <= 0);
    m_amount->clear();
}

void InventoryPage::apply(bool addToStock)
{
    const pos::InventoryRow* row = selectedRow();
    if (!row)
        return;

    bool ok = false;
    const double amount = QLocale::c().toDouble(m_amount->text().trimmed(), &ok);
    const bool whole = row->unit != pos::MeasureUnit::Weight;
    QString problem;
    if (!ok)
        problem = "Type the amount first.";
    else if (addToStock && amount <= 0)
        problem = "The amount to add must be more than 0.";
    else if (!addToStock && amount < 0)
        problem = "The quantity cannot be negative.";
    else if (whole && std::floor(amount) != amount)
        problem = "This product is sold by count, so use a whole number.";
    if (!problem.isEmpty()) {
        m_amount->setState("invalid");
        m_amount->setFocus();
        m_banner->showMessage(problem, Banner::Error, 3500);
        return;
    }

    const std::string id = row->productId;
    const bool saved = addToStock ? m_services.inventory().addStock(id, amount)
                                  : m_services.inventory().setQuantity(id, amount);
    if (!saved) {
        m_banner->showMessage("The stock could not be saved.", Banner::Error, 3500);
        return;
    }

    reload();                                    // keeps the product selected
    const pos::InventoryRow* now = selectedRow();
    const QString total = now ? formatQty(now->quantity, now->unit) : QString();
    m_banner->showMessage(addToStock ? QStringLiteral("Added. Stock is now %1.").arg(total)
                                     : QStringLiteral("Stock set to %1.").arg(total),
                          Banner::Success);
}
