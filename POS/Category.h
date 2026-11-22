#pragma once
#include <optional>
#include <string>
#include <vector>

namespace pos {

    // ---- Layer 1: model ----
    struct Category {
        std::string id, name, acronym;   // id is 10..99 (kCategoryIdDigits digits)
    };

    // ---- Layer 2: storage interface ----
    class ICategoryRepository {
    public:
        virtual ~ICategoryRepository() = default;
        // nullopt if no category has this id
        virtual std::optional<Category> findById(const std::string& id) const = 0;
        // every valid category, in file order
        virtual std::vector<Category> all() const = 0;
        // appends a new category; false if the write fails
        virtual bool add(const Category& c) = 0;
        // replaces name and acronym of the category with the same id;
        // false if the id is not found or the write fails
        virtual bool update(const Category& c) = 0;
    };

    // ---- Layer 2: PCategory.txt implementation ----
    class FileCategoryRepository : public ICategoryRepository {
    public:
        explicit FileCategoryRepository(std::string path);

        std::optional<Category> findById(const std::string& id) const override;
        std::vector<Category> all() const override;
        bool add(const Category& c) override;
        bool update(const Category& c) override;

        // (added, not in the shared interface) messages about malformed lines
        // from the most recent read, e.g. "record 3: expected 3 fields".
        const std::vector<std::string>& warnings() const;

    private:
        std::string path_;
        mutable std::vector<std::string> warnings_;  // mutable: all() is const
    };

    // ---- Layer 3: rules ----
    enum class CategoryError {
        None, BadId, DuplicateId, EmptyName, BadAcronym, DuplicateAcronym, NotFound, SaveFailed
    };

    struct CategoryResult {
        CategoryError error = CategoryError::None;
        std::string message;
        bool ok() const { return error == CategoryError::None; }
    };

    class CategoryService {
    public:
        explicit CategoryService(ICategoryRepository& repo);

        // id must be 10..99 and unused; name not empty; acronym valid and unused
        CategoryResult add(const Category& c);
        // changes name and acronym only; NotFound if the id does not exist
        CategoryResult edit(const Category& c);
        // lowest unused id from 10 to 99, empty string if all are used
        std::string nextFreeId() const;

    private:
        ICategoryRepository& repo_;
    };

} // namespace pos