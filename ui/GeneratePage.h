#pragma once

#include <QImage>
#include <QString>
#include <QWidget>

class Banner;
class QLabel;
class QLineEdit;
class QPushButton;

// Page 1: type a 6-digit number -> barcode is drawn, shown and saved as PNG.
class GeneratePage : public QWidget {
    Q_OBJECT
public:
    explicit GeneratePage(QWidget* parent = nullptr);

    void focusInput();
    void clearPreview();
    // Folder where generated barcodes are saved automatically (next to the app).
    static QString outputDir();

signals:
    void backRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void onTextChanged(const QString& text);
    void generate();
    void randomNumber();
    void saveAs();
    void copyImage();
    void openFolder();
    void updatePreview();

    QLineEdit* m_input = nullptr;
    QLabel* m_hint = nullptr;
    QLabel* m_counter = nullptr;
    QPushButton* m_generateBtn = nullptr;
    QPushButton* m_randomBtn = nullptr;

    QLabel* m_preview = nullptr;
    QLabel* m_digitsLabel = nullptr;
    QLabel* m_path = nullptr;
    Banner* m_banner = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_folderBtn = nullptr;

    QImage m_image;             // the barcode currently shown
    QString m_currentDigits;    // the number it encodes
};
