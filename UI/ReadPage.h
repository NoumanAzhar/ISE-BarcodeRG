#pragma once

#include "PageBase.h"

#include <QPixmap>
#include <QString>

namespace pos { class AppServices; }

class Banner;
class CameraScannerWidget;
class DropZone;
class NeuButton;
class NeuSegmented;
class ProductDetailsView;
class QLabel;
class QStackedWidget;

// ImageAndCamera: manager's "Read" (image file or camera).
// CameraOnly:     cashier's "Scan Product" (camera only).
enum class ReadMode { ImageAndCamera, CameraOnly };

// Reads a barcode from an image or the camera and shows its digits together
// with the product specification from Product.txt.
class ReadPage : public PageBase {
    Q_OBJECT
public:
    ReadPage(pos::AppServices& services, ReadMode mode, QWidget* parent = nullptr);

    void onActivated() override;
    void onDeactivated() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* buildSourceCard();
    QWidget* buildDropZone();
    QWidget* buildResultCard();

    void setSource(int index);              // 0 = image, 1 = camera
    void browse();
    void loadFile(const QString& path);
    void showImage();
    void showDigits(const QString& digits);
    void onCameraScanned(const QString& digits);
    void onCameraFailed(const QString& message);
    void clearResult();
    void clearAll();
    void copyDigits();
    void showError(const QString& text);

    pos::AppServices& m_services;
    ReadMode m_mode;
    bool m_active = false;                  // between onActivated and onDeactivated

    // source card
    NeuSegmented* m_source = nullptr;       // null in CameraOnly mode
    QStackedWidget* m_stack = nullptr;      // null in CameraOnly mode
    CameraScannerWidget* m_camera = nullptr;
    DropZone* m_drop = nullptr;
    QWidget* m_placeholder = nullptr;
    QLabel* m_image = nullptr;
    QLabel* m_caption = nullptr;

    // result card
    QLabel* m_value = nullptr;
    ProductDetailsView* m_details = nullptr;
    Banner* m_banner = nullptr;
    NeuButton* m_copyBtn = nullptr;

    QPixmap m_pixmap;                       // the image currently shown
    QString m_digits;                       // the digits currently shown
    QString m_lastDir;                      // start folder of the file dialog
};
