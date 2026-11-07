#include <QApplication>
#include <QLabel>
#include <opencv2/opencv.hpp>
#include <ZXing/ReadBarcode.h>

int main(int argc , char** argv)
{
    QApplication a(argc , argv);
    QLabel l(QString("OpenCV %1").arg(CV_VERSION));
    l.show();
    return a.exec();
}