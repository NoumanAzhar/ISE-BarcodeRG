#include "ProductEditDialog.h"

#include "AppServices.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"
#include "code128.h"
#include "barcode_render.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include <filesystem>
#include <system_error>
#include <random>
#include <set>

ProductEditDialog::ProductEditDialog(pos::AppServices& services,
                                     std::optional<pos::Product> product,
                                     QWidget* parent)
    : QDialog(parent), m_services(services), m_original(product)
{
    const bool editing = m_original.has_value();
    if (editing)
        m_sold = m_services.orders().containsProduct(m_original->id);

    setWindowTitle(editing ? "Edit Product" : "Add Product");
    setMinimumWidth(480);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    NeuFrame* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(10);

    cardLayout->addWidget(makeLabel(editing ? "Edit Product Details" : "Add New Product",
                                    "sectionTitle", card));

    if (editing) {
        auto* note = makeLabel(
            m_sold ? "Name and acronym are locked because this product has been sold. "
                     "Measurement unit and barcode can never be changed."
                   : "Name and acronym can be corrected until the first sale. "
                     "Measurement unit and barcode can't be changed.",
            "hint", card);
        note->setWordWrap(true);
        cardLayout->addWidget(note);
    }

    // Name
    cardLayout->addWidget(makeLabel("Product Name *", "fieldLabel", card));
    m_name = new NeuLineEdit(card);
    m_name->setPlaceholderText("e.g. Lays Masala");
    cardLayout->addWidget(m_name);

    // Acronym & Unit in a horizontal row
    auto* row1 = new QHBoxLayout;
    row1->setSpacing(12);

    auto* colAcronym = new QVBoxLayout;
    colAcronym->addWidget(makeLabel("Acronym (2-8 letters/digits) *", "fieldLabel", card));
    m_acronym = new NeuLineEdit(card);
    m_acronym->setPlaceholderText("e.g. LAYS");
    m_acronym->setMaxLength(int(pos::kMaxAcronym));
    m_acronym->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9]*"), m_acronym));
    connect(m_acronym, &QLineEdit::textEdited, this, [this](const QString& text) {
        const int cursor = m_acronym->cursorPosition();
        m_acronym->setText(text.toUpper());          // acronyms are always upper case
        m_acronym->setCursorPosition(cursor);
    });
    colAcronym->addWidget(m_acronym);

    auto* colUnit = new QVBoxLayout;
    colUnit->addWidget(makeLabel("Measurement Unit", "fieldLabel", card));
    m_unit = new NeuSegmented(QStringList{ "Count", "Weight" }, card);
    colUnit->addWidget(m_unit);

    row1->addLayout(colAcronym, 1);
    row1->addLayout(colUnit, 1);
    cardLayout->addLayout(row1);

    // Price & Tax
    auto* row2 = new QHBoxLayout;
    row2->setSpacing(12);

    auto* colPrice = new QVBoxLayout;
    colPrice->addWidget(makeLabel("Unit Price (PKR) *", "fieldLabel", card));
    m_price = new NeuLineEdit(card);
    m_price->setPlaceholderText("0.00");
    colPrice->addWidget(m_price);

    auto* colTax = new QVBoxLayout;
    colTax->addWidget(makeLabel("Tax (PKR)", "fieldLabel", card));
    m_tax = new NeuLineEdit(card);
    m_tax->setPlaceholderText("0.00");
    colTax->addWidget(m_tax);

    row2->addLayout(colPrice, 1);
    row2->addLayout(colTax, 1);
    cardLayout->addLayout(row2);

    // Discount & Stock
    auto* row3 = new QHBoxLayout;
    row3->setSpacing(12);

    auto* colDiscount = new QVBoxLayout;
    colDiscount->addWidget(makeLabel("Discount per unit (PKR)", "fieldLabel", card));
    m_discount = new NeuLineEdit(card);
    m_discount->setPlaceholderText("0.00");
    colDiscount->addWidget(m_discount);

    auto* colStock = new QVBoxLayout;
    colStock->addWidget(makeLabel("Available Stock *", "fieldLabel", card));
    m_stock = new NeuLineEdit(card);
    m_stock->setPlaceholderText("0");
    colStock->addWidget(m_stock);

    row3->addLayout(colDiscount, 1);
    row3->addLayout(colStock, 1);
    cardLayout->addLayout(row3);

    // Barcode + Random button
    cardLayout->addWidget(makeLabel(editing ? "Barcode (locked)" : "Barcode (6 digits, optional)",
                                    "fieldLabel", card));
    auto* barRow = new QHBoxLayout;
    barRow->setSpacing(8);
    m_barcode = new NeuLineEdit(card);
    m_barcode->setPlaceholderText(editing ? "Not assigned (use Assign barcodes)"
                                          : "e.g. 100004 (or click Random)");
    m_barcode->setMaxLength(6);
    m_barcode->setValidator(new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,5}"), m_barcode));
    barRow->addWidget(m_barcode, 1);
    if (!editing) {                      // the barcode of an existing product is never changed
        m_randomBtn = new NeuButton("Random", NeuButton::Variant::Normal, card);
        m_randomBtn->setToolTip("Generate an unused random 6-digit barcode");
        barRow->addWidget(m_randomBtn);
    }
    cardLayout->addLayout(barRow);

    // Locked fields (Edit mode): unit and barcode always, name and acronym once sold.
    if (editing) {
        if (m_sold) {
            const QString tip = "This product has been sold, so its name and acronym can't be changed.";
            m_name->setLocked(true);
            m_name->setToolTip(tip);
            m_acronym->setLocked(true);
            m_acronym->setToolTip(tip);
        }
        m_unit->setLocked(true);
        m_unit->setToolTip("The measurement unit can't be changed.");
        m_barcode->setLocked(true);
        m_barcode->setToolTip("The barcode can't be changed.");
    }

    m_banner = new Banner(card);
    cardLayout->addWidget(m_banner);

    // Bottom action buttons
    auto* cancelBtn = new NeuButton("Cancel", NeuButton::Variant::Normal, this);
    cancelBtn->setAutoDefault(false);
    auto* saveBtn = new NeuButton(editing ? "Save Changes" : "Add Product",
                                  NeuButton::Variant::Primary, this);
    saveBtn->setDefault(true);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(8);
    if (editing) {
        m_deleteBtn = new NeuButton("Delete Product", NeuButton::Variant::Danger, this);
        m_deleteBtn->setAutoDefault(false);
        if (m_sold) {
            m_deleteBtn->setEnabled(false);
            m_deleteBtn->setToolTip("Products that have been sold can't be deleted.");
        }
        buttons->addWidget(m_deleteBtn);
    }
    buttons->addStretch(1);
    buttons->addWidget(cancelBtn);
    buttons->addWidget(saveBtn);

    layout->addWidget(card);
    layout->addLayout(buttons);

    // Connections to clear invalid states
    connect(m_name, &QLineEdit::textChanged, this, [this]() { m_name->setState(""); });
    connect(m_acronym, &QLineEdit::textChanged, this, [this]() { m_acronym->setState(""); });
    connect(m_price, &QLineEdit::textChanged, this, [this]() { m_price->setState(""); });
    connect(m_tax, &QLineEdit::textChanged, this, [this]() { m_tax->setState(""); });
    connect(m_discount, &QLineEdit::textChanged, this, [this]() { m_discount->setState(""); });
    connect(m_stock, &QLineEdit::textChanged, this, [this]() { m_stock->setState(""); });
    connect(m_barcode, &QLineEdit::textChanged, this, [this]() { m_barcode->setState(""); });

    if (m_randomBtn)
        connect(m_randomBtn, &NeuButton::clicked, this, &ProductEditDialog::generateRandomBarcode);
    if (m_deleteBtn)
        connect(m_deleteBtn, &NeuButton::clicked, this, &ProductEditDialog::onDelete);
    connect(cancelBtn, &NeuButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &NeuButton::clicked, this, &ProductEditDialog::onSave);

    populateFields();
}

void ProductEditDialog::populateFields()
{
    if (m_original) {
        m_name->setText(qs(m_original->name));
        m_acronym->setText(qs(m_original->acronym));
        m_unit->setCurrentIndex(m_original->unit == pos::MeasureUnit::Weight ? 1 : 0);
        m_price->setText(QString::number(m_original->unitPrice, 'f', 2));
        m_tax->setText(QString::number(m_original->tax, 'f', 2));
        m_discount->setText(QString::number(m_original->discount, 'f', 2));
        m_stock->setText(QString::number(m_original->availableQty));
        m_barcode->setText(qs(m_original->barcode));
    } else {
        m_price->setText("0.00");
        m_tax->setText("0.00");
        m_discount->setText("0.00");
        m_stock->setText("0");
    }
}

void ProductEditDialog::clearErrors()
{
    m_banner->clearMessage();
    m_name->setState("");
    m_acronym->setState("");
    m_price->setState("");
    m_tax->setState("");
    m_discount->setState("");
    m_stock->setState("");
    m_barcode->setState("");
}

void ProductEditDialog::generateRandomBarcode()
{
    std::set<std::string> used;
    for (const auto& p : m_services.products().all()) {
        if (!p.barcode.empty())
            used.insert(p.barcode);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(100000, 999999);
    for (int attempts = 0; attempts < 10000; ++attempts) {
        std::string candidate = std::to_string(dis(gen));
        if (used.find(candidate) == used.end()) {
            m_barcode->setText(qs(candidate));
            m_barcode->setState("");
            return;
        }
    }
}

void ProductEditDialog::onSave()
{
    clearErrors();

    // Once sold, name and acronym are locked and keep their stored values.
    const bool identityLocked = m_original && m_sold;
    const std::string currentId = m_original ? m_original->id : "";

    QString name = m_name->text().trimmed();
    QString acronym = m_acronym->text().trimmed().toUpper();
    if (identityLocked) {
        name = qs(m_original->name);
        acronym = qs(m_original->acronym);
    } else {
        if (name.isEmpty()) {
            m_name->setState("invalid");
            m_name->setFocus();
            m_banner->showMessage("Enter the product name", Banner::Error);
            return;
        }

        // An unchanged acronym of an existing product is not checked again (old data may not follow the rules).
        const bool acronymChanged = !m_original || acronym != qs(m_original->acronym).toUpper();
        if (acronymChanged) {
            QString problem;
            if (acronym.isEmpty())
                problem = "Enter a product acronym (e.g. LAYS)";
            else if (acronym.length() < int(pos::kMinAcronym))
                problem = QString("Acronym must be at least %1 characters").arg(pos::kMinAcronym);
            else if (acronym.length() > int(pos::kMaxAcronym))
                problem = QString("Acronym cannot exceed %1 characters").arg(pos::kMaxAcronym);
            else if (!pos::isValidAcronym(ss(acronym)))
                problem = "Acronym can only use letters and digits, with at least one letter";
            else if (const auto owner = m_services.products().findByAcronym(ss(acronym));
                     owner && owner->id != currentId)
                problem = QString("Acronym '%1' is already used by '%2'").arg(acronym, qs(owner->name));

            if (!problem.isEmpty()) {
                m_acronym->setState("invalid");
                m_acronym->setFocus();
                m_banner->showMessage(problem, Banner::Error);
                return;
            }
        }
    }

    bool okPrice = false;
    const double price = m_price->text().trimmed().toDouble(&okPrice);
    if (!okPrice || price < 0.0) {
        m_price->setState("invalid");
        m_price->setFocus();
        m_banner->showMessage("Enter a valid price in PKR (>= 0)", Banner::Error);
        return;
    }

    bool okTax = true;
    double tax = 0.0;
    const QString taxText = m_tax->text().trimmed();
    if (!taxText.isEmpty()) {
        tax = taxText.toDouble(&okTax);
    }
    if (!okTax || tax < 0.0) {
        m_tax->setState("invalid");
        m_tax->setFocus();
        m_banner->showMessage("Enter a valid tax in PKR (>= 0)", Banner::Error);
        return;
    }

    bool okDisc = true;
    double discount = 0.0;
    const QString discText = m_discount->text().trimmed();
    if (!discText.isEmpty()) {
        discount = discText.toDouble(&okDisc);
    }
    if (!okDisc || discount < 0.0) {
        m_discount->setState("invalid");
        m_discount->setFocus();
        m_banner->showMessage("Enter a valid discount in PKR (>= 0)", Banner::Error);
        return;
    }
    if (discount > price + tax) {
        m_discount->setState("invalid");
        m_discount->setFocus();
        m_banner->showMessage("Discount cannot exceed price + tax", Banner::Error);
        return;
    }

    bool okStock = false;
    const double stock = m_stock->text().trimmed().toDouble(&okStock);
    if (!okStock || stock < 0.0) {
        m_stock->setState("invalid");
        m_stock->setFocus();
        m_banner->showMessage("Enter a valid available stock (>= 0)", Banner::Error);
        return;
    }

    std::string barcode;
    std::string barcodeFile;

    if (!m_original) {                                   // Add mode only: Edit never touches the barcode
        barcode = ss(m_barcode->text().trimmed());
        if (!barcode.empty()) {
            if (!validate6Digits(barcode)) {
                m_barcode->setState("invalid");
                m_barcode->setFocus();
                m_banner->showMessage("Barcode must be 6 digits and start with 1-9", Banner::Error);
                return;
            }
            for (const auto& p : m_services.products().all()) {
                if (p.barcode == barcode) {
                    m_barcode->setState("invalid");
                    m_barcode->setFocus();
                    m_banner->showMessage(QString("Barcode already assigned to '%1'").arg(qs(p.name)), Banner::Error);
                    return;
                }
            }
        }

        // Last look before the name, acronym and unit are saved.
        if (!confirmNewProduct(name, acronym, m_unit->currentIndex() == 0))
            return;

        if (!barcode.empty()) {
            const std::string path = "data/barcodes/" + barcode + ".png";
            std::error_code ec;
            std::filesystem::create_directories("data/barcodes", ec);
            if (!std::filesystem::exists(path)) {
                cv::Mat img = renderBarcode(encodeCode128C(barcode));
                if (!img.empty() && saveBarcode(img, path))
                    barcodeFile = path;
            } else {
                barcodeFile = path;
            }
        }
    }

    pos::Product prod;
    if (m_original)
        prod = *m_original;                              // keeps unit, barcode and barcode image

    if (!identityLocked) {
        prod.name = ss(name);
        prod.acronym = ss(acronym);
    }
    if (!m_original) {
        prod.unit = (m_unit->currentIndex() == 0) ? pos::MeasureUnit::Count : pos::MeasureUnit::Weight;
        prod.barcode = barcode;
        prod.barcodeFile = barcodeFile;
    }
    prod.unitPrice = price;
    prod.tax = tax;
    prod.discount = discount;
    prod.availableQty = stock;

    bool saved = false;
    if (m_original) {
        saved = m_services.products().update(prod);
        m_savedId = prod.id;
    } else {
        saved = m_services.products().add(prod);
        m_savedId = prod.id;
    }

    if (!saved) {
        m_banner->showMessage("Failed to save product to Product.txt", Banner::Error);
        return;
    }

    accept();
}

bool ProductEditDialog::confirmNewProduct(const QString& name, const QString& acronym, bool counted)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle("Check the details");
    box.setText(QString("Name: %1\nAcronym: %2\nUnit: %3")
                    .arg(name, acronym, counted ? "count" : "weight"));
    box.setInformativeText("The measurement unit can never be changed. The name and acronym "
                           "can only be corrected until the product's first sale.");
    QPushButton* back = box.addButton("Go back", QMessageBox::RejectRole);
    QPushButton* add = box.addButton("Add Product", QMessageBox::AcceptRole);
    box.setDefaultButton(back);
    box.exec();
    return box.clickedButton() == add;
}

void ProductEditDialog::onDelete()
{
    if (!m_original || m_sold)
        return;

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle("Delete product");
    box.setText(QString("Delete '%1'?").arg(qs(m_original->name)));
    box.setInformativeText("This product has never been sold. Deleting it can't be undone.");
    QPushButton* keep = box.addButton("Cancel", QMessageBox::RejectRole);
    QPushButton* del = box.addButton("Delete", QMessageBox::DestructiveRole);
    box.setDefaultButton(keep);
    box.exec();
    if (box.clickedButton() != del)
        return;

    if (!m_services.products().remove(m_original->id)) {
        m_banner->showMessage("Failed to delete the product from Product.txt", Banner::Error);
        return;
    }

    // Remove its barcode picture too, unless another product still uses the same file.
    const std::string file = m_original->barcodeFile;
    if (!file.empty()) {
        bool shared = false;
        for (const auto& p : m_services.products().all())
            shared = shared || p.barcodeFile == file;
        if (!shared) {
            std::error_code ec;
            std::filesystem::remove(file, ec);
        }
    }

    m_deleted = true;
    m_savedId.clear();
    accept();
}

