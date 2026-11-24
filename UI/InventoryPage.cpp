#include "InventoryPage.h"

#include "AppServices.h"
#include "DataTable.h"
#include "Format.h"
#include "Neu.h"
#include "ProductDetailsView.h"
#include "ProductEditDialog.h"
#include "Widgets.h"
#include "access.h"
#include "barcode_assign.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

enum Filter { ShowAll, ShowOutOfStock };

constexpr int kNoticeMs = 5000;

bool matches(const pos::InventoryRow& row, const QString& needle)
{
    return qs(row.name).toLower().contains(needle)
        || qs(row.acronym).toLower().contains(needle)
        || qs(row.categoryName).toLower().contains(needle);
}

}

InventoryPage::InventoryPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent), m_services(services)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);
    root->addWidget(buildListSide(), 3);
    root->addWidget(buildDetailsCard(), 2);

    showSelected();
}

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
    m_addBtn = new NeuButton("Add Product", NeuButton::Variant::Primary, side);

    auto* top = new QHBoxLayout;
    top->setSpacing(2);
    top->addWidget(m_search, 1);
    top->addWidget(m_filter);
    top->addWidget(m_addBtn);

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
    connect(m_table, &DataTable::activated, this, &InventoryPage::editSelected);
    connect(m_addBtn, &NeuButton::clicked, this, &InventoryPage::addProduct);
    return side;
}

QWidget* InventoryPage::buildDetailsCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    m_details = new ProductDetailsView(card);

    auto* paper = new QFrame(card);
    paper->setObjectName("paper");
    paper->setStyleSheet("QFrame#paper { background: #ffffff; border-radius: 14px; }");
    auto* paperLayout = new QVBoxLayout(paper);
    paperLayout->setContentsMargins(14, 12, 14, 12);

    m_image = new QLabel(paper);
    m_image->setAlignment(Qt::AlignCenter);
    m_image->setMinimumSize(200, 90);
    m_image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_image->installEventFilter(this);
    paperLayout->addWidget(m_image);

    auto* noImage = makeLabel("No barcode image", "muted", card);
    noImage->setAlignment(Qt::AlignCenter);
    noImage->setMinimumHeight(114);

    m_imageStack = new QStackedWidget(card);
    m_imageStack->addWidget(paper);
    m_imageStack->addWidget(noImage);

    m_banner = new Banner(card);

    m_editBtn = new NeuButton("Edit Product", NeuButton::Variant::Normal, card);
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &NeuButton::clicked, this, &InventoryPage::editSelected);

    auto* assignBtn = new NeuButton("Assign barcodes", NeuButton::Variant::Primary, card);
    assignBtn->setToolTip("Creates a barcode for every product that does not have one yet.");
    assignBtn->setVisible(pos::isAllowed(pos::Role::Manager, pos::Action::GenerateBarcode));
    connect(assignBtn, &NeuButton::clicked, this, &InventoryPage::assignBarcodes);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(8);
    buttons->addWidget(m_editBtn, 1);
    buttons->addWidget(assignBtn, 1);

    layout->addWidget(m_details, 1);
    layout->addWidget(m_imageStack);
    layout->addWidget(m_banner);
    layout->addLayout(buttons);
    return card;
}

bool InventoryPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_image && event->type() == QEvent::Resize)
        showBarcodeImage();
    return PageBase::eventFilter(watched, event);
}

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

void InventoryPage::reload()
{
    m_rows = m_services.inventory().list();
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
    m_table->selectKey(keep);
}

const pos::InventoryRow* InventoryPage::selectedRow() const
{
    const std::string id = ss(m_table->selectedKey());
    const auto it = std::find_if(m_rows.begin(), m_rows.end(),
                                 [&id](const pos::InventoryRow& r) { return r.productId == id; });
    return (id.empty() || it == m_rows.end()) ? nullptr : &*it;
}

void InventoryPage::showSelected()
{
    const pos::InventoryRow* row = selectedRow();
    const auto product = row ? m_services.products().findById(row->productId) : std::nullopt;

    m_editBtn->setEnabled(product.has_value());
    if (!product) {
        m_details->showMessage("Select a product");
        m_barcodePixmap = QPixmap();
    } else {
        m_details->showProduct(*product, qs(row->categoryName));
        m_barcodePixmap = QPixmap(qs(product->barcodeFile));
    }
    showBarcodeImage();
}

void InventoryPage::showBarcodeImage()
{
    const bool has = !m_barcodePixmap.isNull();
    m_imageStack->setCurrentIndex(has ? 0 : 1);
    if (has)
        m_image->setPixmap(fitPixmap(m_barcodePixmap, m_image->size()));
}

void InventoryPage::addProduct()
{
    ProductEditDialog dialog(m_services, std::nullopt, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    reload();
    m_table->selectKey(qs(dialog.savedProductId()));
    if (!dialog.savedNotice().isEmpty())
        m_banner->showMessage(dialog.savedNotice(), Banner::Warning, kNoticeMs);
    else
        m_banner->showMessage("Product added.", Banner::Success);
}

void InventoryPage::editSelected()
{
    const pos::InventoryRow* row = selectedRow();
    if (!row)
        return;
    const auto product = m_services.products().findById(row->productId);
    if (!product)
        return;

    const QString id = qs(row->productId);
    ProductEditDialog dialog(m_services, *product, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    reload();
    m_table->selectKey(id);
    m_banner->showMessage("Changes saved.", Banner::Success);
}

void InventoryPage::assignBarcodes()
{
    const pos::AssignReport report = m_services.assigner().assignAll();
    reload();

    if (report.failed > 0) {
        const QString first = report.messages.empty() ? QString("Some barcodes could not be assigned")
                                                      : qs(report.messages.front());
        m_banner->showMessage(first, Banner::Warning, kNoticeMs);
    } else {
        m_banner->showMessage(QString("%1 assigned, %2 kept").arg(report.assigned).arg(report.kept),
                              Banner::Success);
    }
}
