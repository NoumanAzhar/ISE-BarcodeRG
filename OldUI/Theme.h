#pragma once

#include <QColor>
#include <QString>

// Colours and the application style sheet (dark and light themes).
namespace Theme {

struct Colors {
    QColor bg;           // window background
    QColor surface;      // cards
    QColor surface2;     // inputs, hovered cards
    QColor border;
    QColor text;
    QColor muted;        // secondary text
    QColor accent;
    QColor accentHover;
    QColor danger;
    QColor success;
};

bool isDark();
void setDark(bool dark);
const Colors& colors();      // colours of the current theme
QString styleSheet();        // Qt style sheet for the current theme

} // namespace Theme
