#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

// Member 3: image preprocessing for reading.

// Takes a bar code image (color or gray, loaded from a file), converts to
// grayscale, thresholds, finds the bar code, picks a scanline and returns
// run lengths in pixels, alternating bar / space / bar / ...
// starting with the first BAR (quiet zone is skipped).
// Returns an empty vector if no bar code-like pattern is found.
std::vector<int> extractRuns(const cv::Mat& img);
