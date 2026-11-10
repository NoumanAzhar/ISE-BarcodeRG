#include "MainWindow.h"

#include <QApplication>

int main(int argc , char** argv)
{
    QApplication app(argc , argv);
    QApplication::setStyle("Fusion");               // same look on every platform
    QApplication::setApplicationName("BarcodeRG");
    QApplication::setOrganizationName("BarcodeRG"); // used to remember the theme

    MainWindow window;
    window.show();

    return app.exec();
}
