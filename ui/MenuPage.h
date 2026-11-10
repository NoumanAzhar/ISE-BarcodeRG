#pragma once

#include <QWidget>

// Main menu: two big cards (Generate / Read) plus About and Exit.
class MenuPage : public QWidget {
    Q_OBJECT
public:
    explicit MenuPage(QWidget* parent = nullptr);

signals:
    void generateRequested();
    void readRequested();
    void aboutRequested();
    void exitRequested();
};
