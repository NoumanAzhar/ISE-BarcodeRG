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
	inline constexpr size_t      kMaxAcronym = 8;

	// One line of a data file = list of decoded fields.
	using Record = std::vector<std::string>;

	// Trims leading/trailing spaces; empty string -> "-".
	std::string encodeField(const std::string& value);

	// Reverse of encodeField: trims spaces; "-" -> "".
	std::string decodeField(const std::string& token);

	// One line -> fields. Trims trailing whitespace/'\r', removes the final '#'
	// and the '|' before it, splits on '|', decodes each field.
	Record splitRecord(const std::string& line);

	std::string joinRecord(const Record& fields);

	std::vector<Record> readRecords(const std::string& path);

	bool writeRecords(const std::string& path, const std::vector<Record>& rows);

	bool appendRecord(const std::string& path, const Record& row);

} // namespace pos