#include "MainWindow.h"

#include "HomePage.h"
#include "Roles.h"
#include "Theme.h"
#include "Widgets.h"
#include "Workspace.h"

#include <QCloseEvent>
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
#include<QGuiApplication>
#include<QScreen>
#include<QStyle>

namespace {

// Short fade when a screen appears.
void fadeIn(QWidget* screen)
{
    auto* effect = new QGraphicsOpacityEffect(screen);
    screen->setGraphicsEffect(effect);

    auto* animation = new QPropertyAnimation(effect, "opacity", screen);
    animation->setDuration(200);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    QObject::connect(animation, &QPropertyAnimation::finished, screen,
                     [screen]() { screen->setGraphicsEffect(nullptr); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace

MainWindow::MainWindow(pos::AppServices& services, QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("BarcodeRG");
    resize(1180, 760);
    setMinimumSize(1000, 660);

    if ( const QScreen* screen = QGuiApplication::primaryScreen() )
        setGeometry(QStyle::alignedRect(Qt::LeftToRight , Qt::AlignCenter ,
            size() , screen->availableGeometry()));

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

    m_brand = new QWidget(topBar);                  // logo + name, hidden on Home
    auto* brandLayout = new QHBoxLayout(m_brand);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(4);
    auto* brandBtn = new QPushButton("BarcodeRG", m_brand);
    brandBtn->setObjectName("brandBtn");
    brandBtn->setCursor(Qt::PointingHandCursor);
    brandBtn->setToolTip("Back to Home (Esc)");
    brandLayout->addWidget(new IconWidget(IconWidget::Logo, 30, m_brand));
    brandLayout->addWidget(brandBtn);

    m_roleLabel = makeLabel(QString(), "sectionTitle", topBar);

    m_switchBtn = new QPushButton("Switch role", topBar);
    m_switchBtn->setObjectName("ghost");
    m_switchBtn->setCursor(Qt::PointingHandCursor);

    m_themeBtn = new QPushButton(topBar);
    m_themeBtn->setObjectName("ghost");
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setToolTip("Switch between light and dark theme");

    topLayout->addWidget(m_brand);
    topLayout->addSpacing(14);
    topLayout->addWidget(m_roleLabel);
    topLayout->addStretch(1);
    topLayout->addWidget(m_switchBtn);
    topLayout->addWidget(m_themeBtn);
    layout->addWidget(topBar);

    // ---- screens ----
    m_stack = new QStackedWidget(central);
    m_home = new HomePage(m_stack);
    m_cashier = Roles::buildCashier(services, m_stack);
    m_manager = Roles::buildManager(services, m_stack);
    m_stack->addWidget(m_home);
    m_stack->addWidget(m_cashier);
    m_stack->addWidget(m_manager);
    layout->addWidget(m_stack, 1);

    // ---- navigation ----
    connect(m_home, &HomePage::cashierRequested, this, [this]() { openScreen(m_cashier); });
    connect(m_home, &HomePage::managerRequested, this, [this]() { openScreen(m_manager); });
    connect(m_home, &HomePage::exitRequested, this, &QWidget::close);

    connect(brandBtn, &QPushButton::clicked, this, [this]() { openScreen(m_home); });
    connect(m_switchBtn, &QPushButton::clicked, this, [this]() { openScreen(m_home); });

    // Esc goes back to Home from a workspace.
    auto* escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this]() { openScreen(m_home); });

    // ---- theme ----
    QSettings settings;
    applyTheme(settings.value("theme/dark", false).toBool());
    connect(m_themeBtn, &QPushButton::clicked, this, [this]() { applyTheme(!Theme::isDark()); });

    m_stack->setCurrentWidget(m_home);
    updateTopBar();
}

bool MainWindow::openScreen(QWidget* target)
{
    QWidget* current = m_stack->currentWidget();
    if (current == target)
        return true;

    if (auto* leaving = qobject_cast<Workspace*>(current)) {
        if (!leaving->canLeave())
            return false;
        leaving->leave();
    }

    if (target == m_home)
        m_home->clearFocusState();

    m_stack->setCurrentWidget(target);
    if (auto* entering = qobject_cast<Workspace*>(target))
        entering->enter();

    updateTopBar();
    fadeIn(target);
    return true;
}

void MainWindow::updateTopBar()
{
    const auto* workspace = qobject_cast<Workspace*>(m_stack->currentWidget());
    m_brand->setVisible(workspace != nullptr);
    m_switchBtn->setVisible(workspace != nullptr);
    m_roleLabel->setText(workspace ? workspace->roleName().toUpper() : QString());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (auto* workspace = qobject_cast<Workspace*>(m_stack->currentWidget())) {
        if (!workspace->canLeave()) {
            event->ignore();
            return;
        }
        workspace->leave();         // stops the camera
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::applyTheme(bool dark)
{
    Theme::apply(dark);
    m_themeBtn->setText(dark ? QString(kSun) + "  Light" : QString(kMoon) + "  Dark");

    QSettings settings;
    settings.setValue("theme/dark", dark);
}
