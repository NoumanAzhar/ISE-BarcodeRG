#pragma once

#include <QColor>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QWidget>

class QPainter;

// Neumorphic building blocks. Every shape is PAINTED (two soft shadows from
// one light source, cached as pixmaps), because a style sheet cannot do that.
//
//   Raised : you can press it (buttons, cards, rail items, the selected segment)
//   Inset  : you type in it or look into it (inputs, camera view, lists, pressed buttons)
//   Flat   : nothing painted
enum class NeuStyle { Raised, Inset, Flat };

namespace Neu {

// Room a surface needs around its body for the shadows to be visible.
int margin(NeuStyle style, int depth);

// Draws a surface whose body is `body`. A Raised surface paints its shadows
// OUTSIDE `body`, so the widget must leave margin(Raised, depth) around it.
// `fill` replaces the theme surface colour (e.g. the accent of a primary button).
void paint(QPainter& painter, const QRect& body, int radius, NeuStyle style, int depth,
           qreal devicePixelRatio, const QColor& fill = QColor());

} // namespace Neu

// Ring drawn around a surface. Colours come from the current theme.
enum class NeuOutline { None, Muted, Accent, Success, Danger };

// ---------------------------------------------------------------------------
// A card or well. Its own contents margin already holds the shadow room, so a
// layout inside only adds the padding it wants.
class NeuFrame : public QFrame {
    Q_OBJECT
public:
    explicit NeuFrame(NeuStyle style = NeuStyle::Raised, int radius = 18, QWidget* parent = nullptr);

    void setNeuStyle(NeuStyle style);
    NeuStyle neuStyle() const { return m_style; }
    void setRadius(int radius);
    void setDepth(int depth);                 // shadow size, default 6
    void setReservedMargin(int px);           // fixed room around the body (-1 = automatic)
    void setOutline(NeuOutline outline, Qt::PenStyle line = Qt::SolidLine);

    // Clickable surface: hand cursor, hover tint, pressed look, keyboard focus
    // ring, Enter / Space. Emits clicked().
    void setInteractive(bool on);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void applyMargins();

    NeuStyle m_style;
    int m_radius;
    int m_depth = 6;
    int m_reserved = -1;
    bool m_interactive = false;
    bool m_pressed = false;
    bool m_keyboardFocus = false;
    NeuOutline m_outline = NeuOutline::None;
    Qt::PenStyle m_outlineLine = Qt::SolidLine;
};

// ---------------------------------------------------------------------------
// Raised push button; looks pressed in while held. At least 44 px tall.
class NeuButton : public QPushButton {
    Q_OBJECT
public:
    enum class Variant { Normal, Primary, Danger };

    explicit NeuButton(const QString& text, Variant variant = Variant::Normal,
                       QWidget* parent = nullptr);
    void setVariant(Variant variant);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    Variant m_variant;
    bool m_keyboardFocus = false;
};

// ---------------------------------------------------------------------------
// Inset text input. Dynamic property "state" = "valid" | "invalid" gives a
// green / red ring (call setState()).
class NeuLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit NeuLineEdit(QWidget* parent = nullptr);

    void setLarge(bool large);                 // big monospace-friendly input
    void setState(const QString& state);       // "valid", "invalid" or "" (none)
    // Read-only look: faded, dashed ring and a padlock. Cannot be focused or edited.
    void setLocked(bool locked);
    bool isLocked() const { return m_locked; }
    void setLockIconVisible(bool visible);     // hide the padlock; the locked behaviour stays

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool m_large = false;
    bool m_locked = false;
    bool m_showLock = true;
};

// ---------------------------------------------------------------------------
// 2 to 4 options in an inset track; the selected one is a raised pill with
// accent text. Mouse, Left / Right keys.
class NeuSegmented : public QWidget {
    Q_OBJECT
public:
    explicit NeuSegmented(const QStringList& labels, QWidget* parent = nullptr);

    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);           // emits currentChanged when it changes
    int count() const { return m_labels.size(); }

    // Same locked look as NeuLineEdit; the selection can then only be set from code.
    void setLocked(bool locked);
    bool isLocked() const { return m_locked; }
    void setLockIconVisible(bool visible);     // hide the padlock; the locked behaviour stays

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    QRect segmentRect(int index) const;

    QStringList m_labels;
    int m_current = 0;
    bool m_keyboardFocus = false;
    bool m_locked = false;
    bool m_showLock = true;
};
