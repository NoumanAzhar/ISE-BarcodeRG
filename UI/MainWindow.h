#pragma once

#include <QMainWindow>

class HomePage;
class QCloseEvent;
class QLabel;
class QPushButton;
class QStackedWidget;
class Workspace;
namespace pos { class AppServices; }

// Top bar (brand, role, switch role, theme) and a stack of three screens:
// Home, Cashier workspace, Manager workspace.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(pos::AppServices& services, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    bool openScreen(QWidget* target);    // false if the visible page asked to stay
    void updateTopBar();
    void applyTheme(bool dark);

    QStackedWidget* m_stack = nullptr;
    HomePage* m_home = nullptr;
    Workspace* m_cashier = nullptr;
    Workspace* m_manager = nullptr;

    QWidget* m_brand = nullptr;
    QLabel* m_roleLabel = nullptr;
    QPushButton* m_switchBtn = nullptr;
    QPushButton* m_themeBtn = nullptr;
};
