#pragma once

#include <QString>

#include <string>

namespace pos { enum class MeasureUnit; }

// Conversions between the Qt-free backend and the UI, and the way money and
// quantities are written on screen. One place, so every screen agrees.
QString qs(const std::string& s);       // backend string -> QString
std::string ss(const QString& s);       // QString -> backend string

QString formatPkr(double amount);                          // "PKR 1,250.50"
QString formatQty(double qty, pos::MeasureUnit unit);      // count "3", weight "1.25"
QString unitName(pos::MeasureUnit unit);                   // "count" / "weight"
