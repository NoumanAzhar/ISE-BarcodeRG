#pragma once
#include <optional>
#include <string>
#include <opencv2/core.hpp>

// Member 5: single entry point for reading a bar code from an image file.

// Loads the image at `path`, runs extractRuns() (image_prep.h) and then
// decodeRuns() (barcode_decode.h), and returns the 6-digit string.
// Returns std::nullopt if the file cannot be loaded or decoding fails

std::optional<std::string> readBarcodeFile(const std::string& path);

// Result of one camera frame: the number (if read) and where the bar code
// was found, so the UI can draw a frame around it.
struct CameraScanResult {
    std::optional<std::string> value;   // 6 digits, empty if not read in this frame
    cv::Rect box;                       // bar code area in frame coordinates (empty = none found)
};

// Camera reading. It first finds WHERE the bar code is, then reads it:
//  1. the frame is cut into small cells; cells with strong left-right edges
//     and weak up-down edges look like bars, touching cells form a region
//     (also tried on a 90-degree turned frame for upright bar codes),
//  2. the tilt of each region is found by testing angles and keeping the one
//     that makes the bars sharpest, and the region is rotated level,
//  3. several scan lines are read, each averaged over a few rows and cut to
//     the part with dense bar edges, with a few thresholding variants,
//  4. if nothing is found, thin strips across the whole frame are tried.
// The checksum in decodeRuns() means a wrong number is practically never returned.
CameraScanResult scanCameraFrame(const cv::Mat& frame);

// Same as scanCameraFrame() but only returns the number.
std::optional<std::string> readBarcodeFrame(const cv::Mat& frame);
