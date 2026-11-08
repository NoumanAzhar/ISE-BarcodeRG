#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <opencv2/opencv.hpp>

// Convert an OpenCV grayscale image to a QImage for display in Qt.
static QImage matToQImage(const cv::Mat& gray)
{
    return QImage(gray.data , gray.cols , gray.rows ,
        static_cast< int >( gray.step ) ,
        QImage::Format_Grayscale8).copy();
}

int main(int argc , char** argv)
{
    QApplication app(argc , argv);

    QWidget window;
    window.setWindowTitle("BarcodeRG - sample");
    auto* layout = new QVBoxLayout(&window);

    auto* input = new QLineEdit;
    input->setPlaceholderText("6-digit number, first digit non-zero");
    input->setMaxLength(6);
    input->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[1-9][0-9]{0,5}") , input));

    auto* button = new QPushButton("Generate");
    auto* preview = new QLabel;
    preview->setAlignment(Qt::AlignCenter);
    preview->setMinimumSize(320 , 120);
    auto* status = new QLabel(QString("Ready (OpenCV %1, Qt %2)")
        .arg(CV_VERSION).arg(qVersion()));

    layout->addWidget(input);
    layout->addWidget(button);
    layout->addWidget(preview);
    layout->addWidget(status);

    QObject::connect(button , &QPushButton::clicked , [=] () {
        const QString text = input->text();
        if ( text.size() != 6 ) {
            status->setText("Enter exactly 6 digits (first digit non-zero).");
            return;
        }

        // PLACEHOLDER drawing: replace with the real encoder output.
        cv::Mat img(100 , 300 , CV_8UC1 , cv::Scalar(255));   // white background
        for ( int i = 0; i < 6; ++i ) {
            int d = text [i].digitValue();
            cv::rectangle(img , cv::Rect(20 + i * 45 , 10 , 5 + d * 3 , 80) ,
                cv::Scalar(0) , cv::FILLED);        // black bar
        }

        cv::imwrite("sample.png" , img);                      // save as PNG
        preview->setPixmap(QPixmap::fromImage(matToQImage(img)));
        status->setText("Saved sample.png next to app.exe");
        });

    window.show();
    return app.exec();
}