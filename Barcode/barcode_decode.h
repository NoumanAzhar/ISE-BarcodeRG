#pragma once
#include <optional>
#include <string>
#include <vector>

// Member 4: runs -> 6 digits.

// Takes alternating bar/space widths (first run is a bar), estimates the
// module width, matches 11-module symbols using patternToSymbol() from
// code128.h, checks Start C / Stop / checksum, and returns the 6-digit
// string. Returns std::nullopt if decoding or the checksum fails.
std::optional<std::string> decodeRuns(const std::vector<int>& runs);
