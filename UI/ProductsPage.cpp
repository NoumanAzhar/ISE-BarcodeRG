#include "ProductsPage.h"

#include "AppServices.h"
#include "Format.h"
#include "Neu.h"
#include "ProductDetailsView.h"
#include "ProductEditDialog.h"
#include "Theme.h"
#include "Widgets.h"
#include "barcode_assign.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QPixmap>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {

enum Column { ColAcronym, ColName, ColPrice, ColStock, ColBarcode, ColCount };

constexpr int kWarningMs = 5000;
constexpr int kAcronymIndent = 10;  // pixels the Acronym values are moved to the right
constexpr int kIconSpacing = 4;     // gap Qt leaves between a cell icon and its text
constexpr int kIconMax = 400;       // widest indent the table icon size allows

// Transparent icon: used as an invisible spacer in front of a cell's text.
// Dark red on the light theme, light red on the dark theme.
QColor stockWarningColor()
{
    return Theme::isDark() ? QColor("#ff7b72") : QColor("#b00000");
}

QIcon spacerIcon(int width)
{
    QPixmap blank(width, 1);
    blank.fill(Qt::transparent);
    return QIcon(blank);
}

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

// True if the widget (or a parent below `stop`) is a button, text input, scroll bar or
// table header: clicks there must not count as "empty space".
bool isOnControl(const QWidget* widget, const QWidget* stop)
{
    for (const QWidget* w = widget; w && w != stop; w = w->parentWidget()) {
        if (qobject_cast<const QAbstractButton*>(w) || qobject_cast<const QLineEdit*>(w)
            || qobject_cast<const QAbstractSlider*>(w) || qobject_cast<const QHeaderView*>(w))
            return true;
    }
    return false;
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
    layout->setSpacing(4);

    auto* topLayout = new QHBoxLayout;
    topLayout->setSpacing(8);

    m_search = new NeuLineEdit(side);
    m_search->setPlaceholderText("Search");
    topLayout->addWidget(m_search, 1);

    m_addBtn = new NeuButton("Add Product", NeuButton::Variant::Primary, side);
    topLayout->addWidget(m_addBtn);

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
    m_table->setIconSize(QSize(kIconMax, 1));      // the invisible icon in the Name cells is the indent
    connect(m_table->horizontalHeader(), &QHeaderView::sectionResized, this,
            [this](int column, int, int) { if (column == ColName) updateNameIndent(); });

    m_emptyLabel = makeLabel(QString(), "muted", well);
    m_emptyLabel->setAlignment(Qt::AlignCenter);

    m_listStack = new QStackedWidget(well);
    m_listStack->addWidget(m_table);             // 0
    m_listStack->addWidget(m_emptyLabel);        // 1
    wellLayout->addWidget(m_listStack);

    layout->addLayout(topLayout);
    layout->addWidget(well, 1);

    connect(m_search, &NeuLineEdit::textChanged, this, &ProductsPage::applyFilter);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ProductsPage::showSelected);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        m_table->selectRow(row);         // the first click of a double click may have unselected it
        editSelected();
    });
    connect(m_addBtn, &NeuButton::clicked, this, &ProductsPage::addProduct);
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

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    m_editBtn = new NeuButton("Edit Product", NeuButton::Variant::Normal, card);
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &NeuButton::clicked, this, &ProductsPage::editSelected);

    auto* assignBtn = new NeuButton("Assign barcodes", NeuButton::Variant::Primary, card);
    connect(assignBtn, &NeuButton::clicked, this, &ProductsPage::assignBarcodes);

    btnRow->addWidget(m_editBtn, 1);
    btnRow->addWidget(assignBtn, 1);

    layout->addWidget(m_details, 1);
    layout->addWidget(m_imageStack);
    layout->addWidget(m_banner);
    layout->addLayout(btnRow);
    return card;
}

// ------------------------------------------------------------- page hooks

void ProductsPage::onActivated()
{
    m_active = true;
    qApp->installEventFilter(this);      // to unselect when empty space is clicked
    reload();

    const size_t bad = m_services.products().warnings().size();
    if (bad > 0)
        m_banner->showMessage(QString("%1 lines in Product.txt could not be read").arg(bad),
                              Banner::Warning, kWarningMs);
    else
        m_banner->clearMessage();
}

void ProductsPage::onDeactivated()
{
    m_active = false;
    qApp->removeEventFilter(this);
}

void ProductsPage::resetState()
{
    m_search->clear();
    selectById(QString());
    m_banner->clearMessage();
}

bool ProductsPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_image && event->type() == QEvent::Resize)
        showBarcodeImage();

    const QEvent::Type type = event->type();
    if (m_active && (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonRelease)
        && watched->isWidgetType()) {
        const auto* widget = static_cast<const QWidget*>(watched);
        const auto* mouse = static_cast<const QMouseEvent*>(event);

        if (mouse->button() == Qt::LeftButton && widget->window() == window()) {
            if (widget == m_table->viewport()) {
                // Click on the selected row again = unselect; click below the rows = unselect.
                const QModelIndex index = m_table->indexAt(mouse->position().toPoint());
                if (type == QEvent::MouseButtonPress) {
                    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
                    const int selectedRow = rows.isEmpty() ? -1 : rows.first().row();
                    m_pressedSelectedRow = (index.isValid() && index.row() == selectedRow) ? selectedRow : -1;
                    if (!index.isValid())
                        m_table->clearSelection();
                } else {
                    if (index.isValid() && index.row() == m_pressedSelectedRow)
                        QTimer::singleShot(0, this, [this]() { m_table->clearSelection(); });
                    m_pressedSelectedRow = -1;
                }
            } else if (type == QEvent::MouseButtonPress && (widget == this || isAncestorOf(widget))
                       && !isOnControl(widget, this)) {
                m_table->clearSelection();               // empty space anywhere else on the page
            }
        }
    }
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
    updateNameIndent();

    if (m_table->rowCount() == 0) {
        m_emptyLabel->setText(m_products.empty() ? "No products in Product.txt"
                                                 : "No product matches your search");
        m_listStack->setCurrentIndex(1);
    } else {
        m_listStack->setCurrentIndex(0);
    }

    selectById(keep);
}

void ProductsPage::changeEvent(QEvent* event)
{
    PageBase::changeEvent(event);
    if (event->type() == QEvent::StyleChange)
        updateStockColors();
}

int ProductsPage::nameIndent() const
{
    // The header centres "Name" in its (stretching) column; the cell text starts after the cell
    // padding (10), the icon and the icon gap. Solve for the icon width that makes both start together.
    const QHeaderView* header = m_table->horizontalHeader();
    const int textWidth = header->fontMetrics().horizontalAdvance(QStringLiteral("Name"));
    const int icon = (header->sectionSize(ColName) - textWidth) / 2 - 10 - kIconSpacing;
    return std::clamp(icon, 1, kIconMax);
}

void ProductsPage::updateNameIndent()
{
    const QIcon icon = spacerIcon(nameIndent());
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (auto* item = m_table->item(row, ColName))
            item->setIcon(icon);
    }
}

void ProductsPage::updateStockColors()
{
    if (!m_table)
        return;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (auto* item = m_table->item(row, ColStock); item && item->data(Qt::UserRole).toBool())
            item->setForeground(stockWarningColor());
    }
}

void ProductsPage::addRow(const pos::Product& p)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    auto* acronym = makeItem(qs(p.acronym));
    acronym->setData(Qt::UserRole, qs(p.id));
    acronym->setIcon(spacerIcon(kAcronymIndent));
    auto* stock = makeItem(formatQty(p.availableQty, p.unit), Qt::AlignRight | Qt::AlignVCenter);
    if (p.availableQty <= 0) {
        stock->setData(Qt::UserRole, true);           // out of stock: coloured by updateStockColors()
        QFont f = stock->font();
        f.setBold(true);
        stock->setFont(f);
        stock->setForeground(stockWarningColor());
    }

    m_table->setItem(row, ColAcronym, acronym);
    auto* name = makeItem(qs(p.name));
    name->setIcon(spacerIcon(nameIndent()));
    m_table->setItem(row, ColName, name);
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

    const bool hasValid = (!id.empty() && it != m_products.end());
    if (m_editBtn)
        m_editBtn->setEnabled(hasValid);

    if (!hasValid) {
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

void ProductsPage::addProduct()
{
    ProductEditDialog dialog(m_services, std::nullopt, this);
    if (dialog.exec() == QDialog::Accepted) {
        reload();
        selectById(qs(dialog.savedProductId()));
        m_banner->showMessage("Product added successfully", Banner::Success);
    }
}

void ProductsPage::editSelected()
{
    const std::string id = ss(selectedId());
    if (id.empty())
        return;

    const auto it = std::find_if(m_products.begin(), m_products.end(),
                                 [&id](const pos::Product& p) { return p.id == id; });
    if (it == m_products.end())
        return;

    ProductEditDialog dialog(m_services, *it, this);
    if (dialog.exec() == QDialog::Accepted) {
        reload();
        selectById(qs(id));
        m_banner->showMessage("Product updated successfully", Banner::Success);
    }
}
