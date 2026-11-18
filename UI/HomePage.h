#pragma once

#include <QWidget>

// First screen: choose the role. Two big cards and a quiet Exit button.
class HomePage : public QWidget {
    Q_OBJECT
public:
    explicit HomePage(QWidget* parent = nullptr);
    void clearFocusState();

signals:
    void cashierRequested();
    void managerRequested();
    void exitRequested();
};
