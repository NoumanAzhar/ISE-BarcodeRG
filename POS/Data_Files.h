#pragma once
#include<string>
#include<vector>

inline constexpr const char* kProductFile = "Product.txt"; // also kOrderFile, kOrderDetailFile, kBillFile
inline constexpr int kBarcodeDigits = 6;
inline constexpr size_t kMaxAcronym = 8;
using Record = std::vector<std::string>;
std::string encodeField(const std::string& value); // space -> '_', empty -> "-"
std::string decodeField(const std::string& token); // reverse
Record splitRecord(const std::string& line); // drops the final '#', splits on spaces, decodes
std::string joinRecord(const Record& fields); // "a b c #"
std::vector<Record> readRecords(const std::string& path); // missing file -> empty
bool writeRecords(const std::string& path, const std::vector<Record>& rows); // temp file + rename
bool appendRecord(const std::string& path, const Record& row);
