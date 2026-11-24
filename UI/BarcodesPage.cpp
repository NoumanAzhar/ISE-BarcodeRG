#include "BarcodesPage.h"

#include "GeneratePage.h"
#include "Neu.h"
#include "ReadPage.h"

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QStringList>
#include <QVBoxLayout>

BarcodesPage::BarcodesPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);

    m_switch = new NeuSegmented(QStringList{ "Read barcode", "Generate barcode" }, this);
    auto* top = new QHBoxLayout;
    top->addWidget(m_switch);
    top->addStretch(1);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(new ReadPage(services, ReadMode::ImageAndCamera, m_stack));   // 0
    m_stack->addWidget(new GeneratePage(m_stack));                                   // 1

    root->addLayout(top);
    root->addWidget(m_stack, 1);

    connect(m_switch, &NeuSegmented::currentChanged, this, [this](int index) {
        if (m_active)
            current()->onDeactivated();
        m_stack->setCurrentIndex(index);
        if (m_active)
            current()->onActivated();
    });
}

PageBase* BarcodesPage::current() const
{
    return static_cast<PageBase*>(m_stack->currentWidget());
}

void BarcodesPage::onActivated()
{
    m_active = true;
    current()->onActivated();
}

void BarcodesPage::onDeactivated()
{
    m_active = false;
    current()->onDeactivated();
}
