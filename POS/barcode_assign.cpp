#include "barcode_assign.h"
#include "barcode_render.h"
#include "code128.h"

#include <cstdio>
#include <filesystem>
#include <random>
#include <set>
#include <system_error>
#include <utility>
using namespace std;

namespace pos {
	namespace {
		constexpr int kRandomDigits = 4;
		constexpr int kPerCategory = 10000;

		bool categoryExists(const ICategoryRepository& categories, const string& id)
		{
			return id.size() == static_cast<size_t>(kCategoryIdDigits) && categories.findById(id).has_value();
		}
	}
	BarcodeAssigner::BarcodeAssigner(IProductRepository& products, const ICategoryRepository& categories, string imageDir) : products_(products), categories_(categories), imageDir_(move(imageDir))
	{
	}
	AssignReport BarcodeAssigner::assignAll()
	{
		AssignReport report;
		const vector<Product> original = products_.all();
		vector<Product> products = original;
		vector<bool> usable(products.size(), false);
		vector<bool> isNew(products.size(), false);
		set<string> used;

		// Pass 1: keep a barcode that is valid, unique and starts with its category id
		for (size_t i = 0; i < products.size(); i++)
		{
			const Product& p = products[i];
			if (!categoryExists(categories_, p.categoryId)) continue;
			const bool prefixOK = p.barcode.compare(0, kCategoryIdDigits, p.categoryId) == 0;
			if (prefixOK && validate6Digits(p.barcode) && used.insert(p.barcode).second) {
				usable[i] = true;
				++report.kept;
			}
		}

        // Pass 2: give the others categoryId + 4 random digits, not used yet
        random_device seed;
        mt19937 engine(seed());
        uniform_int_distribution<int> pick(0, kPerCategory - 1);
        for (size_t i = 0; i < products.size(); ++i) {
            if (usable[i]) continue;
            Product& p = products[i];

            if (!categoryExists(categories_, p.categoryId)) {
                ++report.failed;
                report.messages.push_back("Product " + p.id + " has no valid category");
                continue;
            }

            int inCategory = 0;
            for (const string& code : used) {
                if (code.compare(0, kCategoryIdDigits, p.categoryId) == 0) ++inCategory;
            }
            if (inCategory >= kPerCategory) {
                ++report.failed;
                report.messages.push_back("No free barcode left in category " + p.categoryId);
                continue;
            }

            string code;
            do {
                char tail[16];
                snprintf(tail, sizeof(tail), "%0*d", kRandomDigits, pick(engine));
                code = p.categoryId + tail;
            } while (used.count(code));

            if (!validate6Digits(code)) {
                ++report.failed;
                report.messages.push_back("Category " + p.categoryId + " cannot start a barcode");
                continue;
            }

            p.barcode = code;
            used.insert(code);
            usable[i] = true;
            isNew[i] = true;
        }

        // Pass 3: make sure every barcode has an image, then save the changes
        error_code ec;
        filesystem::create_directories(imageDir_, ec);
        for (size_t i = 0; i < products.size(); ++i) {
            if (!usable[i]) continue;
            Product& p = products[i];
            const string path = imageDir_ + "/" + p.barcode + ".png";

            if (!filesystem::exists(path)) {
                cv::Mat image = renderBarcode(encodeCode128C(p.barcode));
                if (image.empty() || !saveBarcode(image, path)) {
                    ++report.failed;
                    report.messages.push_back("Could not draw image for product " + p.id);
                    continue;
                }
            }
            p.barcodeFile = path;
            const bool changed = p.barcode != original[i].barcode
                || p.barcodeFile != original[i].barcodeFile;
            if (!changed) continue;

            if (products_.update(p)) {
                if (isNew[i]) {
                    ++report.assigned;
                    report.messages.push_back("Product " + p.id + " -> " + p.barcode);
                }
            }
            else {
                ++report.failed;
                report.messages.push_back("Could not save product " + p.id);
            }
        }
        return report;
	}
}