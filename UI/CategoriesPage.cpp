#include "CategoriesPage.h"

#include "AppServices.h"
#include "DataTable.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"
#include "Category.h"

#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kMaxAcronymLength = 8;

} // namespace

CategoriesPage::CategoriesPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent), m_services(services)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room

    m_table = new DataTable(this);
    m_table->setColumns({
        { "ID", DataTable::Align::Left, false },
        { "Name", DataTable::Align::Left, true },
        { "Acronym", DataTable::Align::Left, false },
        { "Products", DataTable::Align::Right, false },
    });
    m_table->setEmptyText("No categories yet.\nAdd the first one on the right.");

    root->addWidget(m_table, 3);
    root->addWidget(buildForm(), 2);

    connect(m_table, &DataTable::selectionChanged, this, [this](const QString& key) {
        if (key.isEmpty())
            enterNewMode();
        else
            enterEditMode(key);
    });

    enterNewMode();
}

// ---------------------------------------------------------------- building

QWidget* CategoriesPage::buildForm()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(6);

    m_formTitle = makeLabel("NEW CATEGORY", "sectionTitle", card);

    m_id = new NeuLineEdit(card);
    m_id->setLocked(true);
    m_id->setToolTip("The ID is the first two digits of the barcodes in this category.");

    m_name = new NeuLineEdit(card);
    m_name->setPlaceholderText("e.g. Snacks");

    m_acronym = new NeuLineEdit(card);
    m_acronym->setPlaceholderText("e.g. SNK");
    m_acronym->setMaxLength(kMaxAcronymLength);
    m_acronym->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9]*"), m_acronym));
    connect(m_acronym, &QLineEdit::textEdited, this, [this](const QString& text) {
        const int cursor = m_acronym->cursorPosition();
        m_acronym->setText(text.toUpper());      // acronyms are always upper case
        m_acronym->setCursorPosition(cursor);
    });

    auto* hint = makeLabel("Barcodes start with the category ID: ID 10 gives barcodes like 100001. "
                           "IDs cannot be changed and categories cannot be deleted.",
                           "muted", card);
    hint->setWordWrap(true);

    m_banner = new Banner(card);

    m_newBtn = new NeuButton("New category", NeuButton::Variant::Normal, card);
    m_saveBtn = new NeuButton("Add category", NeuButton::Variant::Primary, card);
    m_saveBtn->setDefault(true);
    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    buttons->addWidget(m_newBtn);
    buttons->addStretch(1);
    buttons->addWidget(m_saveBtn);

    layout->addWidget(m_formTitle);
    layout->addWidget(makeField("Category ID", m_id, card));
    layout->addWidget(makeField("Name", m_name, card));
    layout->addWidget(makeField("Acronym", m_acronym, card));
    layout->addSpacing(4);
    layout->addWidget(hint);
    layout->addStretch(1);
    layout->addWidget(m_banner);
    layout->addLayout(buttons);

    connect(m_name, &QLineEdit::textChanged, this, [this]() { m_name->setState(QString()); });
    connect(m_acronym, &QLineEdit::textChanged, this, [this]() { m_acronym->setState(QString()); });
    connect(m_name, &QLineEdit::returnPressed, this, &CategoriesPage::save);
    connect(m_acronym, &QLineEdit::returnPressed, this, &CategoriesPage::save);
    connect(m_newBtn, &NeuButton::clicked, this, &CategoriesPage::startNew);
    connect(m_saveBtn, &NeuButton::clicked, this, &CategoriesPage::save);
    return card;
}

// ------------------------------------------------------------- page hooks

void CategoriesPage::onActivated()
{
    reload();
}

void CategoriesPage::resetState()
{
    m_banner->clearMessage();
    startNew();
}

// -------------------------------------------------------------------- list

void CategoriesPage::reload()
{
    const QString keep = m_table->selectedKey();

    QHash<QString, int> counts;                  // products per category
    for (const pos::Product& p : m_services.products().all())
        ++counts[qs(p.categoryId)];

    std::vector<pos::Category> categories = m_services.categories().all();
    std::sort(categories.begin(), categories.end(),
              [](const pos::Category& a, const pos::Category& b) { return a.id < b.id; });

    m_table->clearRows();
    for (const pos::Category& c : categories) {
        const QString id = qs(c.id);
        const int count = counts.value(id);
        const int row = m_table->addRow({ id, qs(c.name), qs(c.acronym), QString::number(count) }, id);
        if (count == 0)
            m_table->setTone(row, 3, DataTable::Tone::Muted);
    }
    m_table->selectKey(keep);                    // refreshes the form too
}

// -------------------------------------------------------------------- form

void CategoriesPage::clearFieldStates()
{
    m_name->setState(QString());
    m_acronym->setState(QString());
}

void CategoriesPage::enterNewMode()
{
    m_editing = false;
    m_formTitle->setText("NEW CATEGORY");
    clearFieldStates();
    m_name->clear();
    m_acronym->clear();

    const QString next = qs(m_services.categoryService().nextFreeId());
    m_id->setText(next.isEmpty() ? QStringLiteral("none left") : next);
    m_saveBtn->setText("Add category");
    m_saveBtn->setEnabled(!next.isEmpty());
    if (next.isEmpty())
        m_banner->showMessage("All 90 category IDs are in use.", Banner::Warning, 0);
}

void CategoriesPage::enterEditMode(const QString& id)
{
    const std::optional<pos::Category> category = m_services.categories().findById(ss(id));
    if (!category) {
        enterNewMode();
        return;
    }
    m_editing = true;
    m_formTitle->setText("EDIT CATEGORY");
    clearFieldStates();
    m_id->setText(qs(category->id));
    m_name->setText(qs(category->name));
    m_acronym->setText(qs(category->acronym));
    m_saveBtn->setText("Save changes");
    m_saveBtn->setEnabled(true);
}

void CategoriesPage::startNew()
{
    m_table->clearSelection();                   // leaves edit mode through selectionChanged
    enterNewMode();                              // also when nothing was selected
    m_name->setFocus();
}

void CategoriesPage::save()
{
    if (!m_saveBtn->isEnabled())
        return;
    clearFieldStates();

    pos::Category category;
    category.id = ss(m_id->text());
    category.name = ss(m_name->text().trimmed());
    category.acronym = ss(m_acronym->text().trimmed().toUpper());

    const pos::CategoryResult result = m_editing ? m_services.categoryService().edit(category)
                                                 : m_services.categoryService().add(category);
    if (!result.ok()) {
        switch (result.error) {
        case pos::CategoryError::EmptyName:
            m_name->setState("invalid");
            m_name->setFocus();
            break;
        case pos::CategoryError::BadAcronym:
        case pos::CategoryError::DuplicateAcronym:
            m_acronym->setState("invalid");
            m_acronym->setFocus();
            break;
        default:
            break;
        }
        m_banner->showMessage(qs(result.message), Banner::Error, 4000);
        return;
    }

    const QString name = qs(category.name);
    if (m_editing) {
        reload();                                // keeps the row selected
        m_banner->showMessage(QStringLiteral("Saved %1.").arg(name), Banner::Success);
    } else {
        m_table->clearSelection();
        reload();
        enterNewMode();                          // ready for the next one
        m_banner->showMessage(QStringLiteral("%1 added.").arg(name), Banner::Success);
        m_name->setFocus();
    }
}
