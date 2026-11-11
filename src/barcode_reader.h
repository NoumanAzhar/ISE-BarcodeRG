#pragma once
#include <optional>
#include <string>
#include <opencv2/core.hpp>

// Member 5: single entry point for reading a bar code from an image file.

// Loads the image at `path`, runs extractRuns() (image_prep.h) and then
// decodeRuns() (barcode_decode.h), and returns the 6-digit string.
// Returns std::nullopt if the file cannot be loaded or decoding fails.
std::optional<std::string> readBarcodeFile(const std::string& path);

// Same as readBarcodeFile() but for a frame that is already in memory
// (e.g. one frame from the webcam, BGR or gray). Besides the whole frame it
// also tries several horizontal scan lines and centred crops, because a
// camera image has background around the bar code.
std::optional<std::string> readBarcodeFrame(const cv::Mat& frame);
