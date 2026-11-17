#include "Format.h"

#include "Product.h"

#include <QLocale>

QString qs(const std::string& s)
{
    return QString::fromStdString(s);
}

std::string ss(const QString& s)
{
    return s.toStdString();
}

QString formatPkr(double amount)
{
    return "PKR " + QLocale(QLocale::English).toString(amount, 'f', 2);
}

QString formatQty(double qty, pos::MeasureUnit unit)
{
    if (unit == pos::MeasureUnit::Count)
        return QString::number(qRound(qty));

    QString text = QString::number(qty, 'f', 3);
    while (text.endsWith('0'))
        text.chop(1);
    if (text.endsWith('.'))
        text.chop(1);
    return text;
}

QString unitName(pos::MeasureUnit unit)
{
    return unit == pos::MeasureUnit::Count ? "count" : "weight";
}
