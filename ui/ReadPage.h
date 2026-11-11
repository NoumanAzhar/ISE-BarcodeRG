#pragma once

#include <QPixmap>
#include <QString>
#include <QWidget>

#include <memory>

namespace cv { class VideoCapture; }

class Banner;
class DropZone;
class QLabel;
class QPushButton;
class QTimer;

// Page 2: drop or choose a barcode image -> the 6-digit number is decoded and shown.
// A camera button can also scan a barcode live from the webcam.
class ReadPage : public QWidget {
    Q_OBJECT
public:
    explicit ReadPage(QWidget* parent = nullptr);
    ~ReadPage() override;
    void clearPage();

signals:
    void backRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void browse();
    void loadFile(const QString& path);
    void decode(const QString& path);
    void showImage();
    void showResult(const QString& value);
    void clearResult();
    void copyValue(const QString& value);
    void clearAll();

    // webcam
    void startCamera();
    void stopCamera();
    void onCameraTick();

    DropZone* m_drop = nullptr;
    QWidget* m_placeholder = nullptr;
    QLabel* m_image = nullptr;
    QLabel* m_caption = nullptr;

    QLabel* m_resultValue = nullptr;
    QLabel* m_fileName = nullptr;
    Banner* m_banner = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QPushButton* m_cameraBtn = nullptr;

    QPixmap m_pixmap;       // the image currently shown
    QString m_lastValue;    // the last decoded number

    std::unique_ptr<cv::VideoCapture> m_cap;   // non-null while the camera is on
    QTimer* m_cameraTimer = nullptr;
    int m_frameCount = 0;
};
