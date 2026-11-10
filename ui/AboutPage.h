#pragma once

#include <QWidget>

// Short description of the app and how the barcode works.
class AboutPage : public QWidget {
    Q_OBJECT
public:
    explicit AboutPage(QWidget* parent = nullptr);

signals:
    void backRequested();
};
