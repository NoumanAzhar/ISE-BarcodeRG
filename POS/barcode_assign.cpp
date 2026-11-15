#include "barcode_assign.h"
#include "barcode_render.h"
#include "code128.h"

#include <filesystem>
#include <set>
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

		// give the others the next free number
		int next = 100001;
		for (size_t i = 0; i < products.size(); ++i)
		{
			if (usable[i]) continue;

			while (next <= 999999 && used.count(to_string(next)))
			{
				++next;
			}
			if (next > 999999) {
				++report.failed;
				report.messages.push_back("No free barcode left for product " + products[i].id);
				continue;
			}
			products[i].barcode = to_string(next);
			used.insert(products[i].barcode);
			usable[i] = true;
			isNew[i] = true;
			++next;
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