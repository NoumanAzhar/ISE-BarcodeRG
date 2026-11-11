#include "MainWindow.h"

#include "AboutPage.h"
#include "GeneratePage.h"
#include "MenuPage.h"
#include "ReadPage.h"
#include "Theme.h"
#include "Widgets.h"

#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

// Short fade when a page appears.
void fadeIn(QWidget* page)
{
    auto* effect = new QGraphicsOpacityEffect(page);
    page->setGraphicsEffect(effect);

    auto* animation = new QPropertyAnimation(effect, "opacity", page);
    animation->setDuration(220);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    QObject::connect(animation, &QPropertyAnimation::finished, page,
                     [page]() { page->setGraphicsEffect(nullptr); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("BarcodeRG");
    resize(1040, 700);
    setMinimumSize(900, 620);

    {
        IconWidget bigLogo(IconWidget::Logo, 128);
        setWindowIcon(QIcon(bigLogo.grab()));
    }

    auto* central = new QWidget(this);
    central->setObjectName("root");
    central->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(central);

    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ---- top bar ----
    auto* topBar = new QWidget(central);
    topBar->setObjectName("topBar");
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(28, 14, 28, 6);

    auto* logo = new IconWidget(IconWidget::Logo, 30, topBar);
    auto* brandBtn = new QPushButton("BarcodeRG", topBar);
    brandBtn->setObjectName("brandBtn");
    brandBtn->setCursor(Qt::PointingHandCursor);
    brandBtn->setToolTip("Go to the main menu");

    m_themeBtn = new QPushButton(topBar);
    m_themeBtn->setObjectName("ghost");
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setToolTip("Switch between dark and light theme");

    topLayout->addWidget(logo);
    topLayout->addWidget(brandBtn);
    topLayout->addStretch(1);
    topLayout->addWidget(m_themeBtn);
    layout->addWidget(topBar);

    // ---- pages ----
    m_stack = new QStackedWidget(central);
    m_menu = new MenuPage(m_stack);
    m_generate = new GeneratePage(m_stack);
    m_read = new ReadPage(m_stack);
    m_about = new AboutPage(m_stack);
    m_stack->addWidget(m_menu);
    m_stack->addWidget(m_generate);
    m_stack->addWidget(m_read);
    m_stack->addWidget(m_about);
    layout->addWidget(m_stack, 1);

    // ---- navigation ----
    connect(m_menu, &MenuPage::generateRequested, this, [this]() { showPage(m_generate); });
    connect(m_menu, &MenuPage::readRequested, this, [this]() { showPage(m_read); });
    connect(m_menu, &MenuPage::exitRequested, this, &QWidget::close);

    connect(m_generate, &GeneratePage::backRequested, this, [this]() { showPage(m_menu); });
    connect(m_read, &ReadPage::backRequested, this, [this]() { showPage(m_menu); });
    connect(m_about, &AboutPage::backRequested, this, [this]() { showPage(m_menu); });
    connect(brandBtn, &QPushButton::clicked, this, [this]() { showPage(m_menu); });

    // Esc goes back to the menu from any other page.
    auto* escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this]() {
        if (m_stack->currentWidget() != m_menu)
            showPage(m_menu);
    });

    // ---- theme ----
    QSettings settings;
    applyTheme(settings.value("theme/dark", true).toBool());
    connect(m_themeBtn, &QPushButton::clicked, this, [this]() { applyTheme(!Theme::isDark()); });

    m_stack->setCurrentWidget(m_menu);
}

void MainWindow::showPage(QWidget* page)
{
    if (m_stack->currentWidget() == page)
        return;

    m_stack->setCurrentWidget(page);
    fadeIn(page);

    if (page == m_generate)
        m_generate->focusInput();
}

void MainWindow::applyTheme(bool dark)
{
    Theme::setDark(dark);
    qApp->setStyleSheet(Theme::styleSheet());

    m_themeBtn->setText(dark ? QString(kSun) + "  Light" : QString(kMoon) + "  Dark");

    // The painted icons read their colours from Theme, so repaint them.
    const QList<IconWidget*> icons = findChildren<IconWidget*>();
    for (IconWidget* icon : icons)
        icon->update();

    QSettings settings;
    settings.setValue("theme/dark", dark);
}
