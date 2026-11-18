#pragma once
#include <string>
#include <opencv2/opencv.hpp>

// Member 2: turns a module string ('0'/'1') into a grayscale image.

// Draws the barcode: each module is moduleW pixels wide, bars are `height`
// pixels tall, with a quiet zone of 10 white modules on both sides.
// Returns a CV_8UC1 image (black bars on white background).
cv::Mat renderBarcode(const std::string& modules, int moduleW = 3, int height = 100);

// Saves the image to disk (e.g. PNG). Returns true on success.
bool saveBarcode(const cv::Mat& img, const std::string& path);
