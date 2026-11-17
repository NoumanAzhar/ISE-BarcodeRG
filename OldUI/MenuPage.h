#pragma once

#include <QWidget>

class MenuPage : public QWidget {
    Q_OBJECT
public:
    explicit MenuPage(QWidget* parent = nullptr);
    void clearFocusState();

signals:
    void generateRequested();
    void readRequested();
    void exitRequested();
};
