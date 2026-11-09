#include "barcode_reader.h"
#include "image_prep.h"       // Member 3: extractRuns
#include "barcode_decode.h"   // Member 4: decodeRuns
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

    // Reads the file bytes ourselves so paths with spaces / non-ASCII work on Windows
    cv::Mat loadImage(const std::string& pathUtf8) {
        std::ifstream f(std::filesystem::u8path(pathUtf8), std::ios::binary);
        if (!f) return {};
        std::vector<uchar> bytes((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        if (bytes.empty()) return {};
        return cv::imdecode(bytes, cv::IMREAD_UNCHANGED);   // empty Mat if not an image
    }

    std::optional<std::string> tryDecode(const cv::Mat& img) {
        if (img.empty()) return std::nullopt;
        std::vector<int> runs = extractRuns(img);           // Member 3
        if (runs.empty()) return std::nullopt;
        return decodeRuns(runs);                            // Member 4
    }

} // namespace

std::optional<std::string> readBarcodeFile(const std::string& path) {
    cv::Mat img = loadImage(path);
    if (img.empty()) return std::nullopt;
    return tryDecode(img);
}