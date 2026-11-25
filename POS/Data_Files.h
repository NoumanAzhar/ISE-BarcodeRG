#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace pos {

	// ---- Constants (the only place these literals may appear) ----
	inline constexpr const char* kProductFile = "data/Product.txt";
	inline constexpr const char* kOrderFile = "data/Order.txt";
	inline constexpr const char* kOrderDetailFile = "data/OrderDetail.txt";
	inline constexpr const char* kBillFile = "data/Bill.txt";
	inline constexpr int         kBarcodeDigits = 6;
	inline constexpr size_t      kMinAcronym = 2;
	inline constexpr size_t      kMaxAcronym = 8;

	inline constexpr const char* kCategoryFile = "data/PCategory.txt";
	inline constexpr const char* kSeasonFile = "data/Seasons.txt";
	inline constexpr const char* kBillDetailFile = "data/BillDetail.txt";
	inline constexpr const char* kApprovalFile = "data/Approval.txt";
	inline constexpr const char* kBillPdfDir = "data/bills";

	inline constexpr int    kCategoryIdDigits = 2;    // ids 10..99 (a barcode cannot start with 0)
	inline constexpr int    kBarcodeSerialDigits = 4;    // barcode = category id + 4-digit serial
	inline constexpr double kMaxSpecialPercent = 8.0;  // special discount cap
	inline constexpr double kMaxSeasonPercent = 50.0;  // cap for a newly added seasonal discount
	inline constexpr int    kEditWindowDays = 7;    // paid order edit window
	inline constexpr int    kTopN = 10;   // size of the top-sold / top-remaining reports

	// One line of a data file = list of decoded fields.
	using Record = std::vector<std::string>;

	// Trims spaces; inner spaces -> '_'; empty string -> "-".
	std::string encodeField(const std::string& value);

	// Reverse of encodeField: '_' -> space; "-" -> "".
	std::string decodeField(const std::string& token);

	// One line -> fields. Trims whitespace, drops the final '#', splits on spaces,
	// decodes each field ('_' = space, "-" = empty).
	Record splitRecord(const std::string& line);

	std::string joinRecord(const Record& fields);

	std::vector<Record> readRecords(const std::string& path);

	bool writeRecords(const std::string& path, const std::vector<Record>& rows);

	bool appendRecord(const std::string& path, const Record& row);

} // namespace pos