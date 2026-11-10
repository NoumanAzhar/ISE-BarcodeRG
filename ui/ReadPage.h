#pragma once

#include <QPixmap>
#include <QString>
#include <QWidget>

class Banner;
class DropZone;
class QLabel;
class QListWidget;
class QPushButton;

// Page 2: drop or choose a barcode image -> the 6-digit number is decoded and shown.
class ReadPage : public QWidget {
    Q_OBJECT
public:
    explicit ReadPage(QWidget* parent = nullptr);

signals:
    void backRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void browse();
    void loadFile(const QString& path);
    void decode(const QString& path);
    void showImage();
    void showResult(const QString& value);
    void clearResult();
    void copyValue(const QString& value);
    void clearAll();
    void addToHistory(const QString& value, const QString& fileName);

    DropZone* m_drop = nullptr;
    QWidget* m_placeholder = nullptr;
    QLabel* m_image = nullptr;
    QLabel* m_caption = nullptr;

    QLabel* m_resultValue = nullptr;
    QLabel* m_fileName = nullptr;
    Banner* m_banner = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QListWidget* m_history = nullptr;

    QPixmap m_pixmap;       // the image currently shown
    QString m_lastValue;    // the last decoded number
};
