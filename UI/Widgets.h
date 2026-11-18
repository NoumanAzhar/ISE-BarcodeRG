#pragma once

#include "Neu.h"

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
const QChar kCheck(0x2713);
const QChar kCross(0x2715);
const QChar kMultiply(0x00D7);

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

// A QLabel with an object name from the style sheet (see Theme.h).
QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent = nullptr);

// Raised card / inset well (both already hold their shadow room as contents margin).
NeuFrame* makeCard(QWidget* parent = nullptr);
NeuFrame* makeWell(QWidget* parent = nullptr);

// ---------------------------------------------------------------------------
// Painted icon (no image files). Glyphs are drawn in the accent colour; the
// Logo is an accent tile. Scales to any size.
class IconWidget : public QWidget {
    Q_OBJECT
public:
    enum Kind { Logo, Generate, Scan, Cashier, Manager, Order, Products, Camera };
    IconWidget(Kind kind, int size, QWidget* parent = nullptr);

    void setMuted(bool muted);    // grey instead of accent (inactive rail items)

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Kind m_kind;
    bool m_muted = false;
};

class QTimer;

// ---------------------------------------------------------------------------
// Coloured message bar with a leading symbol; hidden until showMessage().
class Banner : public QLabel {
    Q_OBJECT
public:
    enum Kind { Info, Success, Warning, Error };
    explicit Banner(QWidget* parent = nullptr);
    void showMessage(const QString& text, Kind kind, int timeoutMs = 2500);
    void clearMessage();

private:
    QTimer* m_timer = nullptr;
};

// ---------------------------------------------------------------------------
// Title shown above a page (the Workspace uses it; pages do not draw their own).
class PageHeader : public QWidget {
    Q_OBJECT
public:
    explicit PageHeader(const QString& title, QWidget* parent = nullptr);
    void setTitle(const QString& title);

private:
    QLabel* m_title = nullptr;
};

// ---------------------------------------------------------------------------
// Big clickable raised card (home page). Mouse, Enter and Space all work.
class ActionCard : public NeuFrame {
    Q_OBJECT
public:
    ActionCard(IconWidget::Kind icon, const QString& title, const QString& text,
               QWidget* parent = nullptr);
};

// ---------------------------------------------------------------------------
// Inset area that accepts a dropped image file; also emits clicked().
// Shows a dashed ring that turns accent-coloured while a file is dragged over it.
class DropZone : public NeuFrame {
    Q_OBJECT
public:
    explicit DropZone(QWidget* parent = nullptr);

signals:
    void fileDropped(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
};
