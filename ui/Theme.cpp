#include "Theme.h"

namespace {

bool sDark = true;

const Theme::Colors kDarkColors = {
    QColor("#0a0a0a"),   // bg
    QColor("#141414"),   // surface
    QColor("#1c1c1c"),   // surface2
    QColor("#2e2e2e"),   // border
    QColor("#ededed"),   // text
    QColor("#9a9a9a"),   // muted
    QColor("#4f8cff"),   // accent
    QColor("#6b9eff"),   // accentHover
    QColor("#f85149"),   // danger
    QColor("#3fb950")    // success
};

const Theme::Colors kLightColors = {
    QColor("#f3f5fa"),   // bg
    QColor("#ffffff"),   // surface
    QColor("#eef1f7"),   // surface2
    QColor("#d6dce8"),   // border
    QColor("#1b2333"),   // text
    QColor("#62708a"),   // muted
    QColor("#2f6bff"),   // accent
    QColor("#4d81ff"),   // accentHover
    QColor("#d93636"),   // danger
    QColor("#1a9d4a")    // success
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

QString styleSheet()
{
    const Colors& c = colors();

    QString qss = QString::fromLatin1(R"QSS(
* {
    font-family: "Segoe UI", "Inter", "Helvetica Neue", "Noto Sans", sans-serif;
    font-size: 14px;
    color: @text;
}
QMainWindow, QWidget#root { background: @bg; }
QWidget#topBar, QWidget#page { background: transparent; }
QLabel { background: transparent; }
QToolTip { background: @surface2; color: @text; border: 1px solid @border; padding: 6px 8px; }

/* ---- text ---- */
QLabel#title { font-size: 42px; font-weight: 700; }
QLabel#pageTitle { font-size: 28px; font-weight: 700; }
QLabel#subtitle { font-size: 15px; color: @muted; }
QLabel#muted { color: @muted; }
QLabel#footer { color: @muted; font-size: 12px; }
QLabel#fieldLabel { font-size: 15px; font-weight: 600; }
QLabel#cardTitle { font-size: 20px; font-weight: 700; }
QLabel#cardText { font-size: 14px; color: @muted; }
QLabel#cardLink { color: @accent; font-weight: 600; }
QLabel#aboutText { font-size: 15px; }
QLabel#hint { color: @muted; }
QLabel#hint[state="invalid"] { color: @danger; }
QLabel#hint[state="valid"] { color: @success; }
QLabel#resultValue {
    font-family: "Cascadia Mono", "Consolas", "DejaVu Sans Mono", monospace;
    font-size: 46px; font-weight: 700; color: @accent;
}
QLabel#resultValue[empty="true"] { color: @border; }
QLabel#paperText { color: #8a94a6; }
QLabel#paperDigits {
    font-family: "Cascadia Mono", "Consolas", "DejaVu Sans Mono", monospace;
    font-size: 20px; font-weight: 600; color: #1b2333;
}

/* ---- cards ---- */
QFrame#card { background: @surface; border: 1px solid @border; border-radius: 16px; }
QFrame#paper { background: #ffffff; border: none; border-radius: 12px; }
QFrame#actionCard { background: @surface; border: 1px solid @border; border-radius: 18px; }
QFrame#actionCard:hover { background: @surface2; border: 1px solid @accent; }
QFrame#actionCard:focus { border: 2px solid @accent; }
QFrame#dropZone { background: @surface; border: 2px dashed @border; border-radius: 18px; }
QFrame#dropZone:hover { border: 2px dashed @accent; }
QFrame#dropZone[dragging="true"] { background: @accent_bg; border: 2px dashed @accent; }

/* ---- inputs ---- */
QLineEdit {
    background: @surface2; border: 1px solid @border; border-radius: 12px;
    padding: 12px 14px; font-size: 26px; font-weight: 600;
    selection-background-color: @accent; selection-color: #ffffff;
}
QLineEdit:focus { border: 1px solid @accent; }
QLineEdit[state="invalid"] { border: 1px solid @danger; }
QLineEdit[state="valid"] { border: 1px solid @success; }

/* ---- buttons ---- */
QPushButton {
    background: @surface2; border: 1px solid @border; border-radius: 10px;
    padding: 10px 18px; font-weight: 600;
}
QPushButton:hover { border: 1px solid @accent; }
QPushButton:focus { border: 1px solid @accent; }
QPushButton:disabled { color: @muted; background: @surface; border: 1px solid @border; }
QPushButton#primary { background: @accent; border: 1px solid @accent; color: #ffffff; }
QPushButton#primary:hover { background: @accent_hover; border: 1px solid @accent_hover; }
QPushButton#primary:disabled { background: @border; border: 1px solid @border; color: @muted; }
QPushButton#ghost { background: transparent; border: none; color: @muted; padding: 8px 12px; }
QPushButton#ghost:hover { color: @text; }
QPushButton#brandBtn {
    background: transparent; border: none; padding: 6px 8px;
    font-size: 17px; font-weight: 700;
}

/* ---- status banner ---- */
QLabel#banner { border-radius: 10px; padding: 10px 14px; }
QLabel#banner[kind="info"] { background: @accent_bg; color: @text; }
QLabel#banner[kind="success"] { background: @success_bg; color: @success; }
QLabel#banner[kind="error"] { background: @danger_bg; color: @danger; }

/* ---- list ---- */
QListWidget { background: transparent; border: none; outline: none; }
QListWidget::item { background: @surface2; border-radius: 10px; padding: 10px 12px; margin: 3px 0; }
QListWidget::item:hover { background: @border; }
QListWidget::item:selected { background: @accent; color: #ffffff; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: @border; border-radius: 4px; min-height: 30px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
)QSS");

    // Longer token names first, so "@surface2" is not hit by "@surface".
    struct Token { const char* name; QString value; };
    const Token tokens[] = {
        { "@accent_hover", c.accentHover.name() },
        { "@accent_bg",    rgba(c.accent,  sDark ? 40 : 28) },
        { "@success_bg",   rgba(c.success, sDark ? 36 : 28) },
        { "@danger_bg",    rgba(c.danger,  sDark ? 36 : 28) },
        { "@surface2",     c.surface2.name() },
        { "@surface",      c.surface.name() },
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
