#pragma once

#include <QMainWindow>

class AboutPage;
class GeneratePage;
class MenuPage;
class QPushButton;
class QStackedWidget;
class ReadPage;

// Top bar (brand + theme switch) and a stack of pages.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void showPage(QWidget* page);
    void applyTheme(bool dark);

    QStackedWidget* m_stack = nullptr;
    MenuPage* m_menu = nullptr;
    GeneratePage* m_generate = nullptr;
    ReadPage* m_read = nullptr;
    AboutPage* m_about = nullptr;
    QPushButton* m_themeBtn = nullptr;
};
