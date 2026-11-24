#include "Theme.h"

#include <QApplication>

namespace {

bool sDark = false;   // neumorphism reads best light first; saved setting overrides

const Theme::Colors kDarkColors = {
    QColor("#262a32"),   // bg
    QColor("#262a32"),   // surface (== bg)
    QColor("#22262d"),   // surface2 (well)
    QColor("#343a45"),   // border
    QColor("#e9edf3"),   // text
    QColor("#a0a9b9"),   // muted
    QColor("#6f9dff"),   // accent
    QColor("#8fb2ff"),   // accentHover
    QColor("#ff7b72"),   // danger
    QColor("#56d364"),   // success
    QColor("#e3b341"),   // warning
    QColor("#0b1424"),   // onAccent
    QColor(255, 255, 255, 20),   // shadowLight
    QColor(0, 0, 0, 140)         // shadowDark
};

const Theme::Colors kLightColors = {
    QColor("#e3e8f0"),   // bg
    QColor("#e3e8f0"),   // surface (== bg)
    QColor("#dce1ea"),   // surface2 (well)
    QColor("#c5ccd9"),   // border
    QColor("#1b2333"),   // text
    QColor("#55627b"),   // muted
    QColor("#2559e0"),   // accent
    QColor("#3a6df0"),   // accentHover
    QColor("#c62828"),   // danger
    QColor("#13703a"),   // success
    QColor("#8f5500"),   // warning
    QColor("#ffffff"),   // onAccent
    QColor(255, 255, 255, 242),  // shadowLight
    QColor(150, 163, 186, 191)   // shadowDark
};

// "rgba(r, g, b, a)" for use inside a style sheet (alpha 0-255).
QString rgba(const QColor& c, int alpha)
{
    return QString("rgba(%1, %2, %3, %4)")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
}

} // namespace

namespace Theme {

bool isDark() { return sDark; }

void setDark(bool dark) { sDark = dark; }

const Colors& colors() { return sDark ? kDarkColors : kLightColors; }

void apply(bool dark)
{
    setDark(dark);
    qApp->setStyleSheet(styleSheet());
}

QColor blend(const QColor& a, const QColor& b, qreal t)
{
    t = qBound<qreal>(0.0, t, 1.0);
    return QColor::fromRgbF(a.redF()   + (b.redF()   - a.redF())   * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF()  + (b.blueF()  - a.blueF())  * t);
}

QColor withAlpha(QColor c, int alpha)
{
    c.setAlpha(alpha);
    return c;
}

QString styleSheet()
{
    const Colors& c = colors();

    // Shapes of buttons, inputs, cards and the segmented control are PAINTED
    // (Neu.cpp): a style sheet cannot draw two soft shadows. The style sheet
    // therefore only handles text, banners, lists/tables and scroll bars, plus a
    // plain fallback for QPushButton / QLineEdit that are not Neu widgets.
    QString qss = QString::fromLatin1(R"QSS(
* {
    font-family: "Segoe UI", "Inter", "Helvetica Neue", "Noto Sans", sans-serif;
    font-size: 14px;
    color: @text;
}
QMainWindow, QWidget#root, QDialog, QMessageBox { background: @bg; }
QWidget#topBar, QWidget#page, QWidget#rail { background: transparent; }
QLabel { background: transparent; }
QToolTip { background: @bg; color: @text; border: 1px solid @border; padding: 6px 8px; }

/* ---- text ---- */
QLabel#title { font-size: 40px; font-weight: 700; }
QLabel#pageTitle { font-size: 26px; font-weight: 700; }
QLabel#sectionTitle { font-size: 12px; font-weight: 700; color: @muted; }
QLabel#subtitle { font-size: 15px; color: @muted; }
QLabel#muted { color: @muted; }
QLabel#footer { color: @muted; font-size: 12px; }
QLabel#fieldLabel { font-size: 14px; font-weight: 600; }
QLabel#cardTitle { font-size: 20px; font-weight: 700; }
QLabel#cardText { font-size: 14px; color: @muted; }
QLabel#cardLink { color: @accent; font-weight: 600; }
QLabel#hint { color: @muted; }
QLabel#hint[state="invalid"] { color: @danger; }
QLabel#hint[state="valid"] { color: @success; }
QLabel#mono { font-family: "Cascadia Mono", "Consolas", "DejaVu Sans Mono", monospace; }
QLabel#resultValue {
    font-family: "Cascadia Mono", "Consolas", "DejaVu Sans Mono", monospace;
    font-size: 44px; font-weight: 700; color: @accent;
}
QLabel#resultValue[empty="true"] { color: @border; }
QLabel#totalValue {
    font-family: "Cascadia Mono", "Consolas", "DejaVu Sans Mono", monospace;
    font-size: 30px; font-weight: 700;
}

/* ---- receipt paper (white in both themes) ---- */
QLabel#paperInk { color: #1b2333; }
QLabel#paperMuted, QLabel#paperText { color: #6b7689; }
QLabel#paperTitle { color: #1b2333; font-size: 17px; font-weight: 700; }
QLabel#paperTotal {
    color: #1b2333; font-size: 20px; font-weight: 700;
    font-family: "Cascadia Mono", "Consolas", "DejaVu Sans Mono", monospace;
}
QLabel#paperDigits {
    font-family: "Cascadia Mono", "Consolas", "DejaVu Sans Mono", monospace;
    font-size: 20px; font-weight: 600; color: #1b2333;
}

/* ---- status banner (colour + icon, never shadow alone) ---- */
QLabel#banner { border-radius: 12px; padding: 10px 14px; font-weight: 600; }
QLabel#banner[kind="info"] { background: @accent_bg; color: @text; }
QLabel#banner[kind="success"] { background: @success_bg; color: @success; }
QLabel#banner[kind="warning"] { background: @warning_bg; color: @warning; }
QLabel#banner[kind="error"] { background: @danger_bg; color: @danger; }

/* ---- plain fallbacks (Neu widgets paint themselves) ---- */
QPushButton {
    background: @surface2; border: 1px solid @border; border-radius: 12px;
    padding: 10px 18px; font-weight: 600;
}
QPushButton:hover { border: 1px solid @accent; }
QPushButton:disabled { color: @muted; }
QPushButton#primary { background: @accent; border: 1px solid @accent; color: @on_accent; }
QPushButton#ghost, QPushButton#brandBtn { background: transparent; border: none; color: @muted; }

QLineEdit {
    background: @surface2; border: 1px solid @border; border-radius: 12px;
    padding: 10px 14px; font-size: 15px;
    selection-background-color: @accent; selection-color: @on_accent;
    placeholder-text-color: @muted;
}
QLineEdit:focus { border: 1px solid @accent; }
QLineEdit[neu="true"] {
    background: transparent; border: none; padding: 0px;
    font-size: 15px; font-weight: 500;
}
QLineEdit[neu="true"]:focus { border: none; }
QLineEdit[neu="true"][locked="true"] { color: @muted; }
QLineEdit[neu="true"][large="true"] { font-size: 26px; font-weight: 600; }

/* ---- drop-down (same look as the inputs) ---- */
QComboBox {
    background: @surface2; color: @text; border: 1px solid @border; border-radius: 12px;
    padding: 0px 14px; min-height: 44px; font-size: 15px; font-weight: 500;
}
QComboBox:focus, QComboBox:on { border: 1px solid @accent; }
QComboBox:disabled { color: @muted; }
QComboBox::drop-down { border: none; width: 34px; }
QComboBox::down-arrow {
    width: 0px; height: 0px; margin-right: 12px;
    border-left: 5px solid transparent; border-right: 5px solid transparent;
    border-top: 6px solid @muted;
}
QComboBox QAbstractItemView {
    background: @surface2; color: @text; border: 1px solid @border;
    selection-background-color: @accent; selection-color: @on_accent; outline: none; padding: 4px;
}
QComboBox QAbstractItemView::item { min-height: 32px; padding-left: 10px; }

/* ---- lists and tables (put them inside an inset well) ---- */
QListWidget, QListView, QTableWidget, QTableView, QTreeView {
    background: transparent; border: none; outline: none;
    gridline-color: transparent; alternate-background-color: transparent;
    selection-background-color: @accent_bg; selection-color: @text;
}
QListWidget::item { padding: 10px 12px; border-radius: 10px; margin: 2px 0; }
QListWidget::item:hover { background: @hover_bg; }
QListWidget::item:selected { background: @accent_bg; color: @text; }
QTableView::item { padding: 6px 10px; border-bottom: 1px solid @border; }
QTableView::item:hover { background: @hover_bg; }
QTableView::item:selected { background: @accent_bg; color: @text; }
QHeaderView { background: transparent; }
QHeaderView::section {
    background: transparent; color: @muted; border: none;
    border-bottom: 1px solid @border; padding: 8px 10px; font-weight: 600;
}
QTableCornerButton::section { background: transparent; border: none; }

QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: @scroll; border-radius: 4px; min-height: 30px; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: @scroll; border-radius: 4px; min-width: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
)QSS");

    // Longer token names first, so "@surface2" is not hit by "@surface".
    struct Token { const char* name; QString value; };
    const Token tokens[] = {
        { "@accent_hover", c.accentHover.name() },
        { "@accent_bg",    rgba(c.accent,  sDark ? 40 : 30) },
        { "@success_bg",   rgba(c.success, sDark ? 36 : 30) },
        { "@danger_bg",    rgba(c.danger,  sDark ? 36 : 30) },
        { "@warning_bg",   rgba(c.warning, sDark ? 36 : 34) },
        { "@hover_bg",     rgba(c.text,    sDark ? 14 : 12) },
        { "@scroll",       rgba(c.muted,   110) },
        { "@on_accent",    c.onAccent.name() },
        { "@surface2",     c.surface2.name() },
        { "@surface",      c.surface.name() },
        { "@warning",      c.warning.name() },
        { "@border",       c.border.name() },
        { "@accent",       c.accent.name() },
        { "@success",      c.success.name() },
        { "@danger",       c.danger.name() },
        { "@muted",        c.muted.name() },
        { "@text",         c.text.name() },
        { "@bg",           c.bg.name() },
    };
    for (const Token& t : tokens)
        qss.replace(QString::fromLatin1(t.name), t.value);

    return qss;
}

} // namespace Theme
