#include "season.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include "date_utils.h"

namespace pos {

    namespace {

        bool parsePercent(const std::string& text, double& out)
        {
            if (text.empty())
                return false;
            char* end = nullptr;
            out = std::strtod(text.c_str(), &end);
            return end != nullptr && *end == '\0';
        }

        bool parseActive(const std::string& text, bool& out)
        {
            std::string lower;
            for (char c : text)
                lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower == "true") { out = true; return true; }
            if (lower == "false") { out = false; return true; }
            return false;
        }

        // 10 -> "10", 7.5 -> "7.5", 7.25 -> "7.25"
        std::string formatPercent(double percent)
        {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.2f", percent);
            std::string text = buffer;
            while (!text.empty() && text.back() == '0')
                text.pop_back();
            if (!text.empty() && text.back() == '.')
                text.pop_back();
            return text;
        }

        bool percentInRange(double percent)
        {
            return percent > 0 && percent <= 100;   // false for NaN as well
        }

        // A NEW season is held to the lower cap; lines already in the file (up to 100) still load.
        bool newPercentAllowed(double percent)
        {
            return percent > 0 && percent <= kMaxSeasonPercent;
        }

        // One Seasons.txt record -> Season; false (and a reason) if it is not valid.
        bool toSeason(const Record& r, Season& season, std::string& problem)
        {
            if (r.size() < 4) {
                problem = "expected 4 fields";
                return false;
            }
            if (!isValidDate(r[0]) || !isValidDate(r[1])) {
                problem = "bad date";
                return false;
            }
            if (daysBetween(r[0], r[1]) < 0) {
                problem = "end date is before start date";
                return false;
            }
            double percent = 0;
            if (!parsePercent(r[2], percent) || !percentInRange(percent)) {
                problem = "bad discount percentage";
                return false;
            }
            bool active = false;
            if (!parseActive(r[3], active)) {
                problem = "isActive must be true or false";
                return false;
            }
            season.startDate = r[0];
            season.endDate = r[1];
            season.percent = percent;
            season.active = active;
            return true;
        }

        SeasonResult fail(SeasonError error, const std::string& message)
        {
            SeasonResult result;
            result.error = error;
            result.message = message;
            return result;
        }

    } // namespace

    // ---------------- FileSeasonRepository ----------------

    FileSeasonRepository::FileSeasonRepository(std::string path)
        : path_(std::move(path))
    {
    }

    std::vector<Season> FileSeasonRepository::all() const
    {
        warnings_.clear();
        std::vector<Season> result;
        int number = 0;
        for (const Record& r : readRecords(path_)) {
            ++number;
            Season season;
            std::string problem;
            if (toSeason(r, season, problem))
                result.push_back(season);
            else
                warnings_.push_back("record " + std::to_string(number) + ": " + problem);
        }
        return result;
    }

    bool FileSeasonRepository::add(const Season& season)
    {
        return appendRecord(path_, { season.startDate, season.endDate,
                                     formatPercent(season.percent),
                                     season.active ? "true" : "false" });
    }

    bool FileSeasonRepository::setActive(int index, bool active)
    {
        if (index < 0)
            return false;

        // Work on the raw records so malformed lines are kept exactly as they were.
        std::vector<Record> rows = readRecords(path_);
        int valid = 0;
        for (Record& row : rows) {
            Season season;
            std::string problem;
            if (!toSeason(row, season, problem))
                continue;
            if (valid == index) {
                row[3] = active ? "true" : "false";
                return writeRecords(path_, rows);
            }
            ++valid;
        }
        return false;
    }

    const std::vector<std::string>& FileSeasonRepository::warnings() const
    {
        return warnings_;
    }

    // ---------------- SeasonService ----------------

    SeasonService::SeasonService(ISeasonRepository& repo)
        : repo_(repo)
    {
    }

    SeasonResult SeasonService::add(const Season& season)
    {
        if (!isValidDate(season.startDate) || !isValidDate(season.endDate))
            return fail(SeasonError::BadDate, "Start and end must be real dates like 2026-12-20.");
        if (daysBetween(season.startDate, season.endDate) < 0)
            return fail(SeasonError::EndBeforeStart, "The end date is before the start date.");
        if (!newPercentAllowed(season.percent))
            return fail(SeasonError::BadPercent,
                        "The discount must be more than 0 and at most " +
                        std::to_string(int(kMaxSeasonPercent)) + " percent.");

        for (const Season& existing : repo_.all()) {
            if (existing.startDate == season.startDate && existing.endDate == season.endDate &&
                formatPercent(existing.percent) == formatPercent(season.percent))
                return fail(SeasonError::Duplicate, "This discount already exists.");
        }

        if (!repo_.add(season))
            return fail(SeasonError::SaveFailed, "The discount could not be saved.");
        return SeasonResult();
    }

    bool SeasonService::setActive(int index, bool active)
    {
        if (index < 0 || static_cast<size_t>(index) >= repo_.all().size())
            return false;
        return repo_.setActive(index, active);
    }

    double SeasonService::percentOn(const std::string& date) const
    {
        if (!isValidDate(date))
            return 0;
        double best = 0;
        for (const Season& season : repo_.all()) {
            if (!season.active)
                continue;
            if (inRange(date, { season.startDate, season.endDate }) && season.percent > best)
                best = season.percent;
        }
        return best;
    }

    std::vector<Season> SeasonService::all() const
    {
        return repo_.all();
    }

} // namespace pos
