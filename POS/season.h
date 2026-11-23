#pragma once
#include <string>
#include <vector>
#include "Data_Files.h"

namespace pos {

    // ---- Layer 1: model ----
    // One line of Seasons.txt: startDate endDate discountPercent isActive
    struct Season {
        std::string startDate, endDate;   // yyyy-MM-dd, both inclusive
        double percent = 0;               // 0 < percent <= 100
        bool active = false;
    };

    // ---- Layer 2: storage interface ----
    class ISeasonRepository {
    public:
        virtual ~ISeasonRepository() = default;
        // every valid season, in file order
        virtual std::vector<Season> all() const = 0;
        virtual bool add(const Season& season) = 0;
        // index = position in all(); false if there is no such season or the write fails
        virtual bool setActive(int index, bool active) = 0;
    };

    // ---- Layer 2: Seasons.txt implementation ----
    class FileSeasonRepository : public ISeasonRepository {
    public:
        explicit FileSeasonRepository(std::string path);
        std::vector<Season> all() const override;
        bool add(const Season& season) override;
        bool setActive(int index, bool active) override;
        // (added, not in the shared interface) messages about malformed lines
        // from the most recent all(), e.g. "record 2: bad date".
        const std::vector<std::string>& warnings() const;
    private:
        std::string path_;
        mutable std::vector<std::string> warnings_;   // mutable: all() is const
    };

    // ---- Layer 3: rules ----
    enum class SeasonError { None, BadDate, EndBeforeStart, BadPercent, Duplicate, NotFound, SaveFailed };

    struct SeasonResult {
        SeasonError error = SeasonError::None;
        std::string message;               // ready to show in the GUI
        bool ok() const { return error == SeasonError::None; }
    };

    class SeasonService {
    public:
        explicit SeasonService(ISeasonRepository& repo);
        // 0 < percent <= 100, both dates real, end not before start, not an exact duplicate
        SeasonResult add(const Season& season);
        // index = position in the list the GUI shows (the same order as all())
        SeasonResult setActive(int index, bool active);
        // The highest percent among the active seasons that cover the date
        // (ends included). 0 if none covers it or the date is invalid.
        // Overlapping seasons are not added together.
        double percentOn(const std::string& date) const;
        std::vector<Season> all() const;
    private:
        ISeasonRepository& repo_;
    };

} // namespace pos
