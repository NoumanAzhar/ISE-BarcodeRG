#include "barcode_assign.h"
#include "barcode_render.h"
#include "code128.h"

#include <filesystem>
#include <set>
#include <random>
#include <system_error>
#include <utility>
using namespace std;

namespace pos {
	BarcodeAssigner::BarcodeAssigner(IProductRepository& products, string imageDir) : products_(products), imageDir_(move(imageDir))
	{
	}
	AssignReport BarcodeAssigner::assignAll()
	{
		AssignReport report;
		const vector <Product> original = products_.all();
		vector<Product> products = original;
		vector<bool> usable(products.size(), false);
		vector<bool> isNew(products.size(), false);
		set<string> used;
		// keep every barcode that is not a duplicate 
		for (size_t i = 0; i < products.size(); ++i)
		{
			const string& code = products[i].barcode;
			if (validate6Digits(code) && used.insert(code).second) {
				usable[i] = true;
				++report.kept;
			}
		}

		// give the others a random 6-digit number that is not used yet
		random_device seed;
		mt19937 engine(seed());
		uniform_int_distribution<int> pick(100000, 999999);
		for (size_t i = 0; i < products.size(); ++i)
		{
			if (usable[i]) continue;

			if (used.size() >= 900000) {
				++report.failed;
				report.messages.push_back("No free barcode left for product " + products[i].id);
				continue;
			}
			string code;
			do {
				code = to_string(pick(engine));
			} while (used.count(code));

			products[i].barcode = code;
			used.insert(code);
			usable[i] = true;
			isNew[i] = true;
		}

		// make sure every barcode has an image
		error_code ec;
		filesystem::create_directories(imageDir_, ec);
		for (size_t i = 0; i < products.size(); ++i)
		{
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
			const bool changed = p.barcode != original[i].barcode || p.barcodeFile != original[i].barcodeFile;
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