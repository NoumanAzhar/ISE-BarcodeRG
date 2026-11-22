#pragma once
#include <string>

namespace pos {

    // Calendar helpers on "yyyy-MM-dd" text (the format of Order.txt and Bill.txt).
    // Pure maths: no clock, no time zone, no files. Used by the 7-day edit rule,
    // the season dates and the report periods.

    // true only for a real calendar date: 2028-02-29 yes, 2027-02-29 no.
    bool isValidDate(const std::string& date);

    // 'to' minus 'from' in whole days (negative when 'to' is earlier).
    // Returns 0 if either date is invalid, so check isValidDate first.
    int daysBetween(const std::string& from, const std::string& to);

    // date + days (days may be negative). Empty string if the date is invalid.
    std::string addDays(const std::string& date, int days);

    // Both ends inclusive.
    struct DateRange {
        std::string from, to;
    };

    // true if the date and both ends are valid and from <= date <= to.
    bool inRange(const std::string& date, const DateRange& range);

    enum class Period { Day, Week, Month, Year };   // week = Monday to Sunday

    // The day, week, month or year that contains anchorDate.
    // Both ends empty if anchorDate is invalid.
    DateRange rangeFor(Period period, const std::string& anchorDate);

} // namespace pos
