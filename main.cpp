#include "MainWindow.h"
#include "AppServices.h"

#include <QApplication>
#include <QDir>

namespace
{

    // The backend reads and writes "data/..." relative to the working directory.
    // Use the project folder while developing, the folder of the .exe once the
    // app is deployed.
    void selectWorkingDirectory()
    {
        const QString candidates [] = { QStringLiteral(APP_ROOT), QCoreApplication::applicationDirPath() };
        for ( const QString& dir : candidates ) {
            if ( QDir(dir).exists("data") ) {
                QDir::setCurrent(dir);
                return;
            }
        }
        QDir::setCurrent(QCoreApplication::applicationDirPath());   // data/ is created here
    }

} // namespace

int main(int argc , char** argv)
{
    QApplication app(argc , argv);
    QApplication::setStyle("Fusion");               // same look on every platform
    QApplication::setApplicationName("BarcodeRG");
    QApplication::setOrganizationName("BarcodeRG"); // used to remember the theme

    selectWorkingDirectory();

    pos::AppServices services;
    MainWindow window(services);
    window.show();

    return app.exec();
}
