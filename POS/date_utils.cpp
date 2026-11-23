#include "date_utils.h"
#include <cctype>
#include <cstdio>

namespace pos {

    namespace {

        bool isLeapYear(int year)
        {
            return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        }

        int daysInMonth(int year, int month)
        {
            static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
            return (month == 2 && isLeapYear(year)) ? 29 : days[month - 1];
        }

        int digits(const std::string& text, size_t start, size_t count)
        {
            int value = 0;
            for (size_t i = start; i < start + count; i++)
                value = value * 10 + (text[i] - '0');
            return value;
        }

        // yyyy-MM-dd -> numbers; false if it is not a real date.
        bool parseDate(const std::string& text, int& year, int& month, int& day)
        {
            if (text.size() != 10 || text[4] != '-' || text[7] != '-')
                return false;
            const size_t digitAt[] = { 0, 1, 2, 3, 5, 6, 8, 9 };
            for (size_t i : digitAt) {
                if (!std::isdigit(static_cast<unsigned char>(text[i])))
                    return false;
            }
            year = digits(text, 0, 4);
            month = digits(text, 5, 2);
            day = digits(text, 8, 2);
            if (year < 1 || month < 1 || month > 12)
                return false;
            return day >= 1 && day <= daysInMonth(year, month);
        }

        // Days since 1970-01-01 (proleptic Gregorian calendar).
        long long daysFromCivil(int year, int month, int day)
        {
            year -= month <= 2 ? 1 : 0;
            const long long era = (year >= 0 ? year : year - 399) / 400;
            const long long yearOfEra = year - era * 400;
            const long long dayOfYear = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
            const long long dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
            return era * 146097 + dayOfEra - 719468;
        }

        void civilFromDays(long long days, int& year, int& month, int& day)
        {
            days += 719468;
            const long long era = (days >= 0 ? days : days - 146096) / 146097;
            const long long dayOfEra = days - era * 146097;
            const long long yearOfEra =
                (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
            const long long dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
            const long long monthPart = (5 * dayOfYear + 2) / 153;
            day = static_cast<int>(dayOfYear - (153 * monthPart + 2) / 5 + 1);
            month = static_cast<int>(monthPart < 10 ? monthPart + 3 : monthPart - 9);
            year = static_cast<int>(yearOfEra + era * 400 + (month <= 2 ? 1 : 0));
        }

        std::string formatDate(int year, int month, int day)
        {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month, day);
            return buffer;
        }

    } // namespace

    bool isValidDate(const std::string& date)
    {
        int y, m, d;
        return parseDate(date, y, m, d);
    }

    int daysBetween(const std::string& from, const std::string& to)
    {
        int fy, fm, fd, ty, tm, td;
        if (!parseDate(from, fy, fm, fd) || !parseDate(to, ty, tm, td))
            return 0;
        return static_cast<int>(daysFromCivil(ty, tm, td) - daysFromCivil(fy, fm, fd));
    }

    std::string addDays(const std::string& date, int days)
    {
        int y, m, d;
        if (!parseDate(date, y, m, d))
            return "";
        civilFromDays(daysFromCivil(y, m, d) + days, y, m, d);
        if (y < 1 || y > 9999)
            return "";
        return formatDate(y, m, d);
    }

    bool inRange(const std::string& date, const DateRange& range)
    {
        if (!isValidDate(date) || !isValidDate(range.from) || !isValidDate(range.to))
            return false;
        // yyyy-MM-dd text sorts in date order, so plain comparison is enough
        return range.from <= date && date <= range.to;
    }

    DateRange rangeFor(Period period, const std::string& anchorDate)
    {
        int y, m, d;
        if (!parseDate(anchorDate, y, m, d))
            return { "", "" };

        switch (period) {
        case Period::Day:
            return { anchorDate, anchorDate };
        case Period::Week: {
            // 1970-01-01 was a Thursday, so (days + 3) % 7 counts from Monday = 0
            const long long days = daysFromCivil(y, m, d);
            const int sinceMonday = static_cast<int>(((days + 3) % 7 + 7) % 7);
            const std::string monday = addDays(anchorDate, -sinceMonday);
            return { monday, addDays(monday, 6) };
        }
        case Period::Month:
            return { formatDate(y, m, 1), formatDate(y, m, daysInMonth(y, m)) };
        case Period::Year:
            return { formatDate(y, 1, 1), formatDate(y, 12, 31) };
        }
        return { "", "" };
    }

} // namespace pos
