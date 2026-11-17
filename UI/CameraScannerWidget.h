#pragma once

#include <QPixmap>
#include <QRect>
#include <QString>
#include <QWidget>
#include <QElapsedTimer>

#include <memory>

class CameraReader;
class NeuButton;
class NeuFrame;
class QLabel;
class QStackedLayout;
class QTimer;
struct CameraScanResult;

// Live camera view with a Start / Stop button. The only class that touches
// CameraReader (barcode_reader.h). It keeps scanning while it is on and reports
// every accepted read; the owner decides what a read means.
//
// A read is accepted after two matching results in a row. The same number is
// reported again only after it was out of the picture for about 0.7 s, or when
// 2 s have passed: a held item is not added over and over, but scanning it
// again later counts.
class CameraScannerWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraScannerWidget(QWidget* parent = nullptr);
    ~CameraScannerWidget() override;

    void setActive(bool on);              // start / stop the camera
    bool isActive() const { return m_reader != nullptr; }

signals:
    void barcodeScanned(const QString& digits);   // 6 digits, once per accepted read
    void cameraFailed(const QString& message);
    void activeChanged(bool on);

protected:
    void hideEvent(QHideEvent* event) override;     // never keep the webcam on in the background
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void onTick();
    void handleScan(const CameraScanResult& scan);
    void accept(const QString& digits, const QRect& readBox);
    void drawBoxes(QPixmap& frame) const;
    void showFrame();
    void showPlaceholder(const QString& text);
    void resetScanState();

    std::unique_ptr<CameraReader> m_reader;   // non-null while the camera is on
    QTimer* m_timer = nullptr;
    QElapsedTimer m_clock;

    NeuFrame* m_well = nullptr;
    QStackedLayout* m_stack = nullptr;        // 0 = placeholder, 1 = live picture
    QLabel* m_placeholderText = nullptr;
    QLabel* m_view = nullptr;
    NeuButton* m_toggle = nullptr;

    QPixmap m_frame;                          // newest picture with its boxes drawn
    int m_lastScanId = 0;
    QRect m_box;                              // where the last scan saw a barcode (frame pixels)
    QRect m_readBox;                          // area the last accepted number was read from
    QString m_pending;                        // a number needs two matching reads
    int m_pendingHits = 0;
    int m_missCount = 0;

    QString m_lastEmitted;
    qint64 m_lastSeen = 0;                    // ms, last read of m_lastEmitted
    qint64 m_lastEmitTime = 0;                // ms
    qint64 m_flashUntil = 0;                  // ms, green box shown until then
};
