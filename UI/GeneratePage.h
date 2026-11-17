#pragma once

#include "PageBase.h"

#include <QImage>
#include <QString>

class Banner;
class NeuButton;
class NeuLineEdit;
class QLabel;

// Type a 6-digit number -> the barcode is drawn, shown and saved as PNG.
class GeneratePage : public PageBase {
    Q_OBJECT
public:
    explicit GeneratePage(QWidget* parent = nullptr);

    // Folder where generated barcodes are saved automatically (next to the app).
    static QString outputDir();

    void onActivated() override;    // clear the preview, focus the input

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* buildInputCard();
    QWidget* buildPreviewCard();

    void onTextChanged(const QString& text);
    void generate();
    void randomNumber();
    void saveAs();
    void copyImage();
    void openFolder();
    void updatePreview();
    void clearPreview();
    void setActionsEnabled(bool enabled);

    NeuLineEdit* m_input = nullptr;
    QLabel* m_hint = nullptr;
    QLabel* m_counter = nullptr;
    NeuButton* m_generateBtn = nullptr;

    QLabel* m_preview = nullptr;
    QLabel* m_digitsLabel = nullptr;
    QLabel* m_path = nullptr;
    Banner* m_banner = nullptr;
    NeuButton* m_saveBtn = nullptr;
    NeuButton* m_copyBtn = nullptr;
    NeuButton* m_folderBtn = nullptr;

    QImage m_image;             // the barcode currently shown
    QString m_currentDigits;    // the number it encodes
};
