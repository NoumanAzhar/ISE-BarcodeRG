#include "ProductsPage.h"

#include "AppServices.h"
#include "Format.h"
#include "Neu.h"
#include "ProductDetailsView.h"
#include "Theme.h"
#include "Widgets.h"
#include "barcode_assign.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace {

enum Column { ColAcronym, ColName, ColPrice, ColStock, ColBarcode, ColCount };

constexpr int kWarningMs = 5000;

QTableWidgetItem* makeItem(const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(int(align));
    return item;
}

bool matches(const pos::Product& p, const QString& needle)
{
    return qs(p.acronym).toLower().contains(needle)
        || qs(p.name).toLower().contains(needle)
        || qs(p.barcode).contains(needle);
}

} // namespace

ProductsPage::ProductsPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent), m_services(services)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room
    root->addWidget(buildListSide(), 3);
    root->addWidget(buildDetailsCard(), 2);
}

// ---------------------------------------------------------------- building

QWidget* ProductsPage::buildListSide()
{
    auto* side = new QWidget(this);
    auto* layout = new QVBoxLayout(side);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_search = new NeuLineEdit(side);
    m_search->setPlaceholderText("Search name, acronym or barcode");

    NeuFrame* well = makeWell();
    auto* wellLayout = new QVBoxLayout(well);
    wellLayout->setContentsMargins(8, 8, 8, 8);

    m_table = new QTableWidget(0, ColCount, well);
    m_table->setHorizontalHeaderLabels({ "Acronym", "Name", "Price", "Stock", "Barcode" });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setHighlightSections(false);

    m_emptyLabel = makeLabel(QString(), "muted", well);
    m_emptyLabel->setAlignment(Qt::AlignCenter);

    m_listStack = new QStackedWidget(well);
    m_listStack->addWidget(m_table);             // 0
    m_listStack->addWidget(m_emptyLabel);        // 1
    wellLayout->addWidget(m_listStack);

    layout->addWidget(m_search);
    layout->addWidget(well, 1);

    connect(m_search, &NeuLineEdit::textChanged, this, &ProductsPage::applyFilter);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ProductsPage::showSelected);
    return side;
}

QWidget* ProductsPage::buildDetailsCard()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    m_details = new ProductDetailsView(card);

    // White paper so the barcode image looks like a printed one.
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
    m_imageStack->addWidget(paper);              // 0
    m_imageStack->addWidget(noImage);            // 1

    m_banner = new Banner(card);

    auto* assignBtn = new NeuButton("Assign barcodes", NeuButton::Variant::Primary, card);
    connect(assignBtn, &NeuButton::clicked, this, &ProductsPage::assignBarcodes);

    layout->addWidget(m_details, 1);
    layout->addWidget(m_imageStack);
    layout->addWidget(m_banner);
    layout->addWidget(assignBtn);
    return card;
}

// ------------------------------------------------------------- page hooks

void ProductsPage::onActivated()
{
    reload();

    const size_t bad = m_services.products().warnings().size();
    if (bad > 0)
        m_banner->showMessage(QString("%1 lines in Product.txt could not be read").arg(bad),
                              Banner::Warning, kWarningMs);
    else
        m_banner->clearMessage();
}

bool ProductsPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_image && event->type() == QEvent::Resize)
        showBarcodeImage();
    return PageBase::eventFilter(watched, event);
}

// -------------------------------------------------------------------- list

void ProductsPage::reload()
{
    m_products = m_services.products().all();
    applyFilter();                               // keeps the selected product if it still exists
}

void ProductsPage::applyFilter()
{
    const QString keep = selectedId();
    const QString needle = m_search->text().trimmed().toLower();

    m_table->blockSignals(true);
    m_table->setRowCount(0);
    for (const pos::Product& p : m_products) {
        if (needle.isEmpty() || matches(p, needle))
            addRow(p);
    }
    m_table->blockSignals(false);

    if (m_table->rowCount() == 0) {
        m_emptyLabel->setText(m_products.empty() ? "No products in Product.txt"
                                                 : "No product matches your search");
        m_listStack->setCurrentIndex(1);
    } else {
        m_listStack->setCurrentIndex(0);
    }

    selectById(keep);
}

void ProductsPage::addRow(const pos::Product& p)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    auto* acronym = makeItem(qs(p.acronym));
    acronym->setData(Qt::UserRole, qs(p.id));
    auto* stock = makeItem(formatQty(p.availableQty, p.unit), Qt::AlignRight | Qt::AlignVCenter);
    if (p.availableQty <= 0)
        stock->setForeground(Theme::colors().danger);

    m_table->setItem(row, ColAcronym, acronym);
    m_table->setItem(row, ColName, makeItem(qs(p.name)));
    m_table->setItem(row, ColPrice, makeItem(formatPkr(p.unitPrice), Qt::AlignRight | Qt::AlignVCenter));
    m_table->setItem(row, ColStock, stock);
    m_table->setItem(row, ColBarcode, makeItem(qs(p.barcode)));
}

QString ProductsPage::selectedId() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return QString();
    return m_table->item(rows.first().row(), ColAcronym)->data(Qt::UserRole).toString();
}

void ProductsPage::selectById(const QString& id)
{
    m_table->blockSignals(true);
    m_table->clearSelection();
    for (int row = 0; row < m_table->rowCount() && !id.isEmpty(); ++row) {
        if (m_table->item(row, ColAcronym)->data(Qt::UserRole).toString() == id) {
            m_table->selectRow(row);
            break;
        }
    }
    m_table->blockSignals(false);
    showSelected();
}

// ----------------------------------------------------------------- details

void ProductsPage::showSelected()
{
    const std::string id = ss(selectedId());
    const auto it = std::find_if(m_products.begin(), m_products.end(),
                                 [&id](const pos::Product& p) { return p.id == id; });

    if (id.empty() || it == m_products.end()) {
        m_details->showMessage("Select a product");
        m_barcodePixmap = QPixmap();
    } else {
        m_details->showProduct(*it);
        m_barcodePixmap = QPixmap(qs(it->barcodeFile));
    }
    showBarcodeImage();
}

void ProductsPage::showBarcodeImage()
{
    const bool has = !m_barcodePixmap.isNull();
    m_imageStack->setCurrentIndex(has ? 0 : 1);
    if (has)
        m_image->setPixmap(fitPixmap(m_barcodePixmap, m_image->size()));
}

void ProductsPage::assignBarcodes()
{
    const pos::AssignReport report = m_services.assigner().assignAll();
    reload();

    if (report.failed > 0) {
        const QString first = report.messages.empty() ? QString("Some barcodes could not be assigned")
                                                      : qs(report.messages.front());
        m_banner->showMessage(first, Banner::Warning, kWarningMs);
    } else {
        m_banner->showMessage(QString("%1 assigned, %2 kept").arg(report.assigned).arg(report.kept),
                              Banner::Success);
    }
}
