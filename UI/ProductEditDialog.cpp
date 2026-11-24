#include "ProductEditDialog.h"

#include "AppServices.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"
#include "barcode_assign.h"
#include "Category.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// Two inputs side by side with their captions.
QHBoxLayout* makeRow(QWidget* left, QWidget* right)
{
    auto* row = new QHBoxLayout;
    row->setSpacing(12);
    row->addWidget(left, 1);
    row->addWidget(right, 1);
    return row;
}

} // namespace

ProductEditDialog::ProductEditDialog(pos::AppServices& services,
                                     std::optional<pos::Product> product,
                                     QWidget* parent)
    : QDialog(parent), m_services(services), m_original(product)
{
    const bool editing = m_original.has_value();
    if (editing)
        m_sold = m_services.orders().containsProduct(m_original->id);

    setWindowTitle(editing ? "Edit product" : "Add product");
    setMinimumWidth(520);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    NeuFrame* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(6);

    cardLayout->addWidget(makeLabel(editing ? "EDIT PRODUCT" : "NEW PRODUCT", "sectionTitle", card));

    // ---- name
    m_name = new NeuLineEdit(card);
    m_name->setPlaceholderText("e.g. Lays Classic");
    cardLayout->addWidget(makeField("Product name", m_name, card));

    // ---- acronym and category
    m_acronym = new NeuLineEdit(card);
    m_acronym->setPlaceholderText("e.g. LAYS");
    m_acronym->setMaxLength(int(pos::kMaxAcronym));
    m_acronym->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9]*"), m_acronym));
    connect(m_acronym, &QLineEdit::textEdited, this, [this](const QString& text) {
        const int cursor = m_acronym->cursorPosition();
        m_acronym->setText(text.toUpper());          // acronyms are always upper case
        m_acronym->setCursorPosition(cursor);
    });

    m_category = new QComboBox(card);
    m_category->setEditable(false);
    m_category->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    cardLayout->addLayout(makeRow(makeField("Acronym", m_acronym, card),
                                  makeField("Category", m_category, card)));

    // ---- unit and price
    m_unit = new NeuSegmented(QStringList{ "Count", "Weight" }, card);
    m_price = new NeuLineEdit(card);
    m_price->setPlaceholderText("0.00");
    cardLayout->addLayout(makeRow(makeField("Measurement unit", m_unit, card),
                                  makeField("Unit price (PKR)", m_price, card)));

    // ---- tax and discount
    m_tax = new NeuLineEdit(card);
    m_tax->setPlaceholderText("0.00");
    m_discount = new NeuLineEdit(card);
    m_discount->setPlaceholderText("0.00");
    cardLayout->addLayout(makeRow(makeField("Tax per unit (PKR)", m_tax, card),
                                  makeField("Discount per unit (PKR)", m_discount, card)));

    // ---- stock and barcode
    m_stock = new NeuLineEdit(card);
    m_stock->setPlaceholderText("0");
    if (editing) {
        m_barcode = new NeuLineEdit(card);
        m_barcode->setPlaceholderText("Not created yet");
        cardLayout->addLayout(makeRow(makeField("Available stock", m_stock, card),
                                      makeField("Barcode", m_barcode, card)));
    } else {
        cardLayout->addWidget(makeField("Available stock", m_stock, card));
        auto* hint = makeLabel("The barcode is created for you when you save: it starts with the "
                               "category ID, for example 10 gives 100001.", "muted", card);
        hint->setWordWrap(true);
        cardLayout->addSpacing(2);
        cardLayout->addWidget(hint);
    }

    // ---- locked fields (Edit mode): unit and barcode always, name and acronym once sold,
    //      category once the product has a barcode (the barcode starts with it).
    if (editing) {
        m_name->setLockIconVisible(false);
        m_acronym->setLockIconVisible(false);
        m_unit->setLockIconVisible(false);
        m_barcode->setLockIconVisible(false);
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
        if (!m_original->barcode.empty()) {
            m_category->setEnabled(false);
            m_category->setToolTip("The category can't be changed: the barcode starts with its ID.");
        }
    }

    m_banner = new Banner(card);
    cardLayout->addWidget(m_banner);

    // ---- buttons
    auto* cancelBtn = new NeuButton("Cancel", NeuButton::Variant::Normal, this);
    cancelBtn->setAutoDefault(false);
    m_saveBtn = new NeuButton(editing ? "Save changes" : "Add product", NeuButton::Variant::Primary, this);
    m_saveBtn->setDefault(true);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(8);
    buttons->addStretch(1);
    buttons->addWidget(cancelBtn);
    buttons->addWidget(m_saveBtn);

    layout->addWidget(card);
    layout->addLayout(buttons);

    // Clear a red ring as soon as the field is edited.
    for (NeuLineEdit* edit : { m_name, m_acronym, m_price, m_tax, m_discount, m_stock })
        connect(edit, &QLineEdit::textChanged, this, [edit]() { edit->setState(QString()); });

    connect(cancelBtn, &NeuButton::clicked, this, &QDialog::reject);
    connect(m_saveBtn, &NeuButton::clicked, this, &ProductEditDialog::onSave);

    fillCategories();
    populateFields();
}

void ProductEditDialog::fillCategories()
{
    std::vector<pos::Category> categories = m_services.categories().all();
    std::sort(categories.begin(), categories.end(),
              [](const pos::Category& a, const pos::Category& b) { return a.id < b.id; });

    for (const pos::Category& c : categories)
        m_category->addItem(QString("%1  (%2)").arg(qs(c.name), qs(c.id)), qs(c.id));
    m_category->setPlaceholderText("Select category");
    m_category->setCurrentIndex(-1);

    if (categories.empty()) {
        m_category->setEnabled(false);
        m_saveBtn->setEnabled(false);
        m_banner->showMessage("There are no categories yet. Add one on the Categories page first.",
                              Banner::Warning, 0);
    }
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
        m_category->setCurrentIndex(m_category->findData(qs(m_original->categoryId)));
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
    for (NeuLineEdit* edit : { m_name, m_acronym, m_price, m_tax, m_discount, m_stock })
        edit->setState(QString());
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
            m_banner->showMessage("Enter the product name.", Banner::Error);
            return;
        }

        // An unchanged acronym of an existing product is not checked again (old data may not follow the rules).
        const bool acronymChanged = !m_original || acronym != qs(m_original->acronym).toUpper();
        if (acronymChanged) {
            QString problem;
            if (acronym.isEmpty())
                problem = "Enter a product acronym (for example LAYS).";
            else if (acronym.length() < int(pos::kMinAcronym))
                problem = QString("The acronym needs at least %1 characters.").arg(pos::kMinAcronym);
            else if (acronym.length() > int(pos::kMaxAcronym))
                problem = QString("The acronym can have at most %1 characters.").arg(pos::kMaxAcronym);
            else if (!pos::isValidAcronym(ss(acronym)))
                problem = "Use letters and digits only, with at least one letter.";
            else if (const auto owner = m_services.products().findByAcronym(ss(acronym));
                     owner && owner->id != currentId)
                problem = QString("The acronym %1 is already used by %2.").arg(acronym, qs(owner->name));

            if (!problem.isEmpty()) {
                m_acronym->setState("invalid");
                m_acronym->setFocus();
                m_banner->showMessage(problem, Banner::Error);
                return;
            }
        }
    }

    // A product needs a category until it has a barcode (the barcode is built from it).
    const bool categoryLocked = !m_category->isEnabled();
    if (!categoryLocked && m_category->currentIndex() < 0) {
        m_category->setFocus();
        m_banner->showMessage("Choose a category.", Banner::Error);
        return;
    }

    bool okPrice = false;
    const double price = m_price->text().trimmed().toDouble(&okPrice);
    if (!okPrice || price < 0.0) {
        m_price->setState("invalid");
        m_price->setFocus();
        m_banner->showMessage("Enter a valid price in PKR (0 or more).", Banner::Error);
        return;
    }

    bool okTax = true;
    double tax = 0.0;
    const QString taxText = m_tax->text().trimmed();
    if (!taxText.isEmpty())
        tax = taxText.toDouble(&okTax);
    if (!okTax || tax < 0.0) {
        m_tax->setState("invalid");
        m_tax->setFocus();
        m_banner->showMessage("Enter a valid tax in PKR (0 or more).", Banner::Error);
        return;
    }

    bool okDisc = true;
    double discount = 0.0;
    const QString discText = m_discount->text().trimmed();
    if (!discText.isEmpty())
        discount = discText.toDouble(&okDisc);
    if (!okDisc || discount < 0.0) {
        m_discount->setState("invalid");
        m_discount->setFocus();
        m_banner->showMessage("Enter a valid discount in PKR (0 or more).", Banner::Error);
        return;
    }
    if (discount > price + tax) {
        m_discount->setState("invalid");
        m_discount->setFocus();
        m_banner->showMessage("The discount cannot be more than price plus tax.", Banner::Error);
        return;
    }

    bool okStock = false;
    const double stock = m_stock->text().trimmed().toDouble(&okStock);
    if (!okStock || stock < 0.0) {
        m_stock->setState("invalid");
        m_stock->setFocus();
        m_banner->showMessage("Enter the available stock (0 or more).", Banner::Error);
        return;
    }

    pos::Product prod;
    if (m_original)
        prod = *m_original;                              // keeps unit, barcode and barcode image
    if (!identityLocked) {
        prod.name = ss(name);
        prod.acronym = ss(acronym);
    }
    if (!m_original)
        prod.unit = (m_unit->currentIndex() == 0) ? pos::MeasureUnit::Count : pos::MeasureUnit::Weight;
    if (!categoryLocked)
        prod.categoryId = ss(m_category->currentData().toString());
    prod.unitPrice = price;
    prod.tax = tax;
    prod.discount = discount;
    prod.availableQty = stock;

    const pos::ProductResult result = m_original ? m_services.productService().edit(prod)
                                                 : m_services.productService().add(prod);
    if (!result.ok()) {
        switch (result.error) {
        case pos::ProductError::EmptyName:   m_name->setState("invalid");     m_name->setFocus();     break;
        case pos::ProductError::BadAcronym:  m_acronym->setState("invalid");  m_acronym->setFocus();  break;
        case pos::ProductError::BadPrice:    m_price->setState("invalid");    m_price->setFocus();    break;
        case pos::ProductError::BadDiscount: m_discount->setState("invalid"); m_discount->setFocus(); break;
        case pos::ProductError::BadStock:    m_stock->setState("invalid");    m_stock->setFocus();    break;
        case pos::ProductError::UnknownCategory: m_category->setFocus(); break;
        default: break;
        }
        m_banner->showMessage(qs(result.message), Banner::Error, 5000);
        return;
    }

    m_savedId = m_original ? prod.id : result.product.id;

    // A new product gets its barcode now: category ID + the next free serial.
    if (!m_original) {
        const pos::AssignReport report = m_services.assigner().assignAll();
        if (report.failed > 0) {
            m_notice = report.messages.empty()
                           ? QString("The product was saved, but its barcode could not be created.")
                           : QString("The product was saved, but its barcode could not be created: %1")
                                 .arg(qs(report.messages.front()));
        }
    }

    accept();
}
