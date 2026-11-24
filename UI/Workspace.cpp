#include "Workspace.h"

#include "Neu.h"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// One button of the rail: icon over a short label. Raised when idle, pressed
// in with an accent icon when it is the current page.
class RailItem : public NeuFrame {
    Q_OBJECT
public:
    RailItem(IconWidget::Kind icon, const QString& label, QWidget* parent)
        : NeuFrame(NeuStyle::Raised, 18, parent)
    {
        setDepth(4);
        setReservedMargin(8);          // same room in both looks, so nothing shifts
        setInteractive(true);
        setToolTip(label);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 8, 4, 6);
        layout->setSpacing(3);

        m_icon = new IconWidget(icon, 28, this);
        m_label = makeLabel(label, "muted", this);
        m_label->setAlignment(Qt::AlignCenter);
        m_label->setWordWrap(true);
        m_label->setStyleSheet("font-size: 12px; font-weight: 700;");   // colour stays from Theme

        layout->addWidget(m_icon, 0, Qt::AlignHCenter);
        layout->addWidget(m_label);

        for (QWidget* w : { static_cast<QWidget*>(m_icon), static_cast<QWidget*>(m_label) })
            w->setAttribute(Qt::WA_TransparentForMouseEvents);
        setSelected(false);
    }

    void setSelected(bool selected)
    {
        setNeuStyle(selected ? NeuStyle::Inset : NeuStyle::Raised);
        m_icon->setMuted(!selected);
        m_label->setObjectName(selected ? "cardLink" : "muted");
        repolish(m_label);
    }

private:
    IconWidget* m_icon = nullptr;
    QLabel* m_label = nullptr;
};

// ---------------------------------------------------------------------------

Workspace::Workspace(const QString& roleName, QWidget* parent)
    : QWidget(parent), m_roleName(roleName)
{
    setObjectName("page");

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(20, 8, 28, 22);
    root->setSpacing(18);

    m_rail = new QWidget;
    m_rail->setObjectName("rail");
    m_railLayout = new QVBoxLayout(m_rail);
    m_railLayout->setContentsMargins(0, 0, 0, 0);
    m_railLayout->setSpacing(4);
    m_railLayout->addStretch(1);

    // The rail scrolls (without a scroll bar) when the window is too short for every page.
    auto* railScroll = new QScrollArea(this);
    railScroll->setFixedWidth(104);
    railScroll->setFrameShape(QFrame::NoFrame);
    railScroll->setWidgetResizable(true);
    railScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    railScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    railScroll->setWidget(m_rail);
    railScroll->viewport()->setAutoFillBackground(false);
    m_rail->setAutoFillBackground(false);        // setWidget() switches it back on
    root->addWidget(railScroll);

    auto* right = new QVBoxLayout;
    right->setContentsMargins(0, 6, 0, 0);
    right->setSpacing(12);
    m_header = new PageHeader(QString(), this);
    m_stack = new QStackedWidget(this);
    right->addWidget(m_header);
    right->addWidget(m_stack, 1);
    root->addLayout(right, 1);
}

void Workspace::addPage(const QString& title, IconWidget::Kind icon, PageBase* page)
{
    const int index = int(m_entries.size());

    auto* item = new RailItem(icon, title, m_rail);
    m_railLayout->insertWidget(index, item);          // before the trailing stretch
    page->setParent(m_stack);
    m_stack->addWidget(page);
    m_entries.push_back({ title, item, page });

    connect(item, &NeuFrame::clicked, this, [this, index]() { select(index); });
    connect(page, &PageBase::openPageRequested, this, &Workspace::showPage);

    if (index < 9) {
        auto* shortcut = new QShortcut(QKeySequence(QKeyCombination(Qt::ControlModifier, static_cast<Qt::Key>(Qt::Key_1 + index))), this);
        connect(shortcut, &QShortcut::activated, this, [this, index]() { select(index); });
    }

    if (m_current < 0)
        select(index);     // the first page is shown from the start
}

void Workspace::select(int index)
{
    if (index == m_current || index < 0 || index >= int(m_entries.size()))
        return;
    if (m_current >= 0) {
        if (!m_entries[m_current].page->canLeave())
            return;
        m_entries[m_current].page->onDeactivated();
        m_entries[m_current].item->setSelected(false);
    }

    m_current = index;
    m_entries[index].item->setSelected(true);
    m_header->setTitle(m_entries[index].title);
    m_stack->setCurrentWidget(m_entries[index].page);
    if (isVisible())
        m_entries[index].page->onActivated();
}

void Workspace::showPage(const QString& title)
{
    for (int i = 0; i < int(m_entries.size()); ++i) {
        if (m_entries[i].title == title) {
            select(i);
            return;
        }
    }
}

bool Workspace::canLeave()
{
    return m_current < 0 || m_entries[m_current].page->canLeave();
}

void Workspace::enter()
{
    if (m_current >= 0)
        m_entries[m_current].page->onActivated();
}

void Workspace::leave()
{
    if (m_current >= 0)
        m_entries[m_current].page->onDeactivated();

    // Leaving the role: every page starts clean the next time it is entered.
    for (Entry& entry : m_entries)
        entry.page->resetState();
}

#include "Workspace.moc"
