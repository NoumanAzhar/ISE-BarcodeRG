#pragma once
#include <string>

// Member 1: Code 128 (Set C) encoder + pattern lookup.

// Returns true if s is exactly 6 digits and the first digit is non-zero.
bool validate6Digits(const std::string& s);

// Encodes 6 digits into a module string of '0'/'1' characters
// (Start C + 3 data symbols + checksum + Stop). Example: "11010011100...".
// Returns an empty string if the input is invalid.
std::string encodeCode128C(const std::string& digits);

// Maps an 11-module pattern (e.g. "11010011100") to its Code 128 symbol
// value (0-106). Returns -1 if the pattern is not valid.
int patternToSymbol(const std::string& p11);
