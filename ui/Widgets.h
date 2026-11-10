#pragma once

#include <QChar>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QWidget>

#include <string>

// Non-ASCII characters used in the UI, written as code points so the source
// files stay plain ASCII (no encoding trouble with any compiler).
const QChar kArrowLeft(0x2190);
const QChar kArrowRight(0x2192);
const QChar kEllipsis(0x2026);
const QChar kMiddleDot(0x00B7);
const QChar kSun(0x2600);
const QChar kMoon(0x263E);

// Path as the backend (OpenCV) expects it: the local 8-bit encoding.
inline std::string toNativePath(const QString& path)
{
    return QFile::encodeName(path).toStdString();
}

// Re-applies the style sheet after a dynamic property changed.
void repolish(QWidget* w);

// Scales a pixmap to fit `area`. Small images are enlarged by a whole number
// (crisp bars), big images are shrunk smoothly.
QPixmap fitPixmap(const QPixmap& src, const QSize& area);

// A plain rounded card (styled by the style sheet).
QFrame* makeCard(QWidget* parent = nullptr);

// ---------------------------------------------------------------------------
// Small painted icon (no image files needed): a barcode on a rounded tile.
class IconWidget : public QWidget {
public:
    enum Kind { Logo, Generate, Scan };
    IconWidget(Kind kind, int size, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Kind m_kind;
};

// ---------------------------------------------------------------------------
// Coloured message bar; hidden until showMessage() is called.
class Banner : public QLabel {
public:
    enum Kind { Info, Success, Error };
    explicit Banner(QWidget* parent = nullptr);
    void showMessage(const QString& text, Kind kind);
    void clearMessage();
};

// ---------------------------------------------------------------------------
// Back button + title + subtitle shown at the top of every page.
class PageHeader : public QWidget {
    Q_OBJECT
public:
    PageHeader(const QString& title, const QString& subtitle, QWidget* parent = nullptr);

signals:
    void backClicked();
};

// ---------------------------------------------------------------------------
// Big clickable card used on the main menu (mouse, Enter and Space all work).
class ActionCard : public QFrame {
    Q_OBJECT
public:
    ActionCard(IconWidget::Kind icon, const QString& title, const QString& text,
               QWidget* parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

// ---------------------------------------------------------------------------
// Area that accepts a dropped image file; also emits clicked() when clicked.
class DropZone : public QFrame {
    Q_OBJECT
public:
    explicit DropZone(QWidget* parent = nullptr);

signals:
    void fileDropped(const QString& path);
    void clicked();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
