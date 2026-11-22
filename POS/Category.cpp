#include "Category.h"
#include "Data_Files.h"
#include <cctype>
#include <string>
#include <vector>
using namespace std;

/*

PCategory.txt file

- category id (2 digits, 10 to 99)
- category name
- category acronym (up to 8 letters or digits, at least one letter)

*/

namespace pos {

    // helpers local to this file (everything public is declared in Category.h)
    namespace {

        string upper(string text)
        {
            for (int i = 0; i < text.length(); i++)
                text[i] = static_cast<char>(toupper(static_cast<unsigned char>(text[i])));
            return text;
        }

        bool isBlank(const string& text)
        {
            return text.find_first_not_of(" \t") == string::npos;
        }

        int lowestId()
        {
            int value = 1;
            for (int i = 1; i < kCategoryIdDigits; i++)
                value *= 10;
            return value;                       // 2 digits -> 10
        }

        int highestId()
        {
            return lowestId() * 10 - 1;         // 2 digits -> 99
        }

        string idRangeText()
        {
            return "id must be a number from " + to_string(lowestId()) + " to " + to_string(highestId());
        }

        bool isValidCategoryId(const string& id)
        {
            if (id.length() != static_cast<int>(kCategoryIdDigits))
                return false;
            if (id[0] == '0')
                return false;                   // a barcode cannot start with 0
            for (int i = 0; i < id.length(); i++)
            {
                if (id[i] < '0' || id[i] > '9')
                    return false;
            }
            return true;
        }

        bool isValidCategoryAcronym(const string& acronym)
        {
            if (acronym.empty() || acronym.length() > static_cast<int>(kMaxAcronym))
                return false;
            bool hasLetter = false;
            for (int i = 0; i < acronym.length(); i++)
            {
                const char c = acronym[i];
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                    hasLetter = true;
                else if (c < '0' || c > '9')
                    return false;
            }
            return hasLetter;
        }

        optional<Category> makeCategory(const Record& row, string& error)
        {
            if (row.size() != 3)
            {
                error = "expected 3 fields";
                return nullopt;
            }
            if (!isValidCategoryId(row[0]))
            {
                error = idRangeText();
                return nullopt;
            }
            if (isBlank(row[1]))
            {
                error = "name is empty";
                return nullopt;
            }
            if (!isValidCategoryAcronym(row[2]))
            {
                error = "acronym is not valid";
                return nullopt;
            }
            Category c;
            c.id = row[0];
            c.name = row[1];
            c.acronym = row[2];
            return c;
        }

        Record categoryToRecord(const Category& c)
        {
            Record r;
            r.push_back(c.id);
            r.push_back(c.name);
            r.push_back(c.acronym);
            return r;
        }

    } // anonymous namespace

    // ---- FileCategoryRepository ----

    FileCategoryRepository::FileCategoryRepository(string path)
    {
        this->path_ = path;
    }

    vector<Category> FileCategoryRepository::all() const
    {
        warnings_.clear();
        vector<Record> data = readRecords(path_);
        vector<Category> res;
        for (int i = 0; i < data.size(); i++)
        {
            string error;
            optional<Category> c = makeCategory(data[i], error);
            if (!c)
            {
                warnings_.push_back("record " + to_string(i + 1) + ": " + error);
                continue;
            }
            res.push_back(*c);
        }
        return res;
    }

    optional<Category> FileCategoryRepository::findById(const string& id) const
    {
        if (id == "")
            return nullopt;
        vector<Category> list = all();
        for (int i = 0; i < list.size(); i++)
        {
            if (list[i].id == id)
                return list[i];
        }
        return nullopt;
    }

    bool FileCategoryRepository::add(const Category& c)
    {
        vector<Record> data = readRecords(path_);
        data.push_back(categoryToRecord(c));
        return writeRecords(path_, data);
    }

    bool FileCategoryRepository::update(const Category& c)
    {
        vector<Record> data = readRecords(path_);
        for (int i = 0; i < data.size(); i++)
        {
            if (!data[i].empty() && data[i][0] == c.id)
            {
                data[i] = categoryToRecord(c);
                return writeRecords(path_, data);
            }
        }
        return false;
    }

    const vector<string>& FileCategoryRepository::warnings() const
    {
        return warnings_;
    }

    // ---- CategoryService ----

    CategoryService::CategoryService(ICategoryRepository& repo) : repo_(repo)
    {
    }

    CategoryResult CategoryService::add(const Category& c)
    {
        CategoryResult res;
        if (!isValidCategoryId(c.id))
        {
            res.error = CategoryError::BadId;
            res.message = idRangeText();
            return res;
        }
        if (repo_.findById(c.id))
        {
            res.error = CategoryError::DuplicateId;
            res.message = "a category with this id already exists";
            return res;
        }
        if (isBlank(c.name))
        {
            res.error = CategoryError::EmptyName;
            res.message = "category name is empty";
            return res;
        }
        if (!isValidCategoryAcronym(c.acronym))
        {
            res.error = CategoryError::BadAcronym;
            res.message = "acronym must be up to 8 letters or digits and contain a letter";
            return res;
        }
        vector<Category> existing = repo_.all();
        for (int i = 0; i < existing.size(); i++)
        {
            if (upper(existing[i].acronym) == upper(c.acronym))
            {
                res.error = CategoryError::DuplicateAcronym;
                res.message = "acronym already used";
                return res;
            }
        }
        Category toSave = c;
        toSave.acronym = upper(c.acronym);
        if (!repo_.add(toSave))
        {
            res.error = CategoryError::SaveFailed;
            res.message = "could not save the category";
            return res;
        }
        return res;
    }

    CategoryResult CategoryService::edit(const Category& c)
    {
        CategoryResult res;
        if (!isValidCategoryId(c.id))
        {
            res.error = CategoryError::BadId;
            res.message = idRangeText();
            return res;
        }
        if (!repo_.findById(c.id))
        {
            res.error = CategoryError::NotFound;
            res.message = "category not found";
            return res;
        }
        if (isBlank(c.name))
        {
            res.error = CategoryError::EmptyName;
            res.message = "category name is empty";
            return res;
        }
        if (!isValidCategoryAcronym(c.acronym))
        {
            res.error = CategoryError::BadAcronym;
            res.message = "acronym must be up to 8 letters or digits and contain a letter";
            return res;
        }
        vector<Category> existing = repo_.all();
        for (int i = 0; i < existing.size(); i++)
        {
            if (existing[i].id != c.id && upper(existing[i].acronym) == upper(c.acronym))
            {
                res.error = CategoryError::DuplicateAcronym;
                res.message = "acronym already used";
                return res;
            }
        }
        Category toSave = c;
        toSave.acronym = upper(c.acronym);
        if (!repo_.update(toSave))
        {
            res.error = CategoryError::SaveFailed;
            res.message = "could not save the category";
            return res;
        }
        return res;
    }

    string CategoryService::nextFreeId() const
    {
        vector<Category> list = repo_.all();
        vector<bool> used(highestId() + 1, false);
        for (int i = 0; i < list.size(); i++)
        {
            if (!isValidCategoryId(list[i].id))
                continue;
            used[stoi(list[i].id)] = true;
        }
        for (int id = lowestId(); id <= highestId(); id++)
        {
            if (!used[id])
                return to_string(id);
        }
        return "";
    }

} // namespace pos