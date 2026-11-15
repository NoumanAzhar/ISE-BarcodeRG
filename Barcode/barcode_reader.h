#pragma once
#include <optional>
#include <string>

// Member 5: single entry point for reading a bar code from an image file.

// Loads the image at `path`, runs extractRuns() (image_prep.h) and then
// decodeRuns() (barcode_decode.h), and returns the 6-digit string.
// Returns std::nullopt if the file cannot be loaded or decoding fails.
std::optional<std::string> readBarcodeFile(const std::string& path);
