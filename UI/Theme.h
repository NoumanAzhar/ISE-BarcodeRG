#pragma once

#include <QColor>
#include <QString>

// Colours and the application style sheet (neumorphic dark and light themes).
//
// Neumorphism needs the page and the surfaces to be ONE colour, so `surface`
// is the same as `bg`. Depth comes from two shadows (see Neu.h), not from
// different greys. `surface2` is the slightly darker "well" colour used inside
// inset things (inputs, camera view, lists).
//
// Object names understood by the style sheet (set with setObjectName):
//   text    : title, pageTitle, sectionTitle, subtitle, muted, fieldLabel,
//             cardTitle, cardText, cardLink, hint, resultValue, totalValue,
//             mono, footer
//   receipt : paperInk, paperMuted, paperTitle, paperTotal, paperText,
//             paperDigits   (always dark ink: the paper is white in both themes)
//   banner  : use the Banner widget
//   Dynamic property "state" = "valid" | "invalid" on QLabel#hint and on
//   NeuLineEdit gives green / red.
namespace Theme {

struct Colors {
    QColor bg;           // window background
    QColor surface;      // raised surfaces (== bg)
    QColor surface2;     // inset "well" fill
    QColor border;       // hairlines (table rows, empty states)
    QColor text;
    QColor muted;        // secondary text
    QColor accent;
    QColor accentHover;
    QColor danger;
    QColor success;
    // added for the neumorphic redesign
    QColor warning;
    QColor onAccent;     // text/icons drawn on top of the accent colour
    QColor shadowLight;  // highlight (top-left) shadow, with alpha
    QColor shadowDark;   // dark (bottom-right) shadow, with alpha
};

bool isDark();
void setDark(bool dark);
const Colors& colors();      // colours of the current theme
QString styleSheet();        // Qt style sheet for the current theme

// setDark() + qApp->setStyleSheet(). Painted widgets repaint themselves.
void apply(bool dark);

// Small colour helpers.
QColor blend(const QColor& a, const QColor& b, qreal t);   // t = 0 -> a, 1 -> b
QColor withAlpha(QColor c, int alpha);                      // alpha 0-255

} // namespace Theme
