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
    // Transparent background -> white (otherwise it turns black and hides the bars)
    cv::Mat flattenAlpha(const cv::Mat& bgra) {
        std::vector<cv::Mat> ch;
        cv::split(bgra, ch);
        cv::Mat a, inv;
        ch[3].convertTo(a, CV_32F, 1.0 / 255.0);
        cv::subtract(cv::Scalar(1.0), a, inv);
        std::vector<cv::Mat> out(3);
        for (int i = 0; i < 3; ++i) {
            cv::Mat c;
            ch[i].convertTo(c, CV_32F);
            out[i] = c.mul(a) + inv * 255.0;
        }
        cv::Mat merged;
        cv::merge(out, merged);
        merged.convertTo(merged, CV_8U);
        return merged;
    }

    // Reads the file bytes ourselves so paths with spaces / non-ASCII work on Windows
    cv::Mat loadImage(const std::string& pathUtf8) {
        std::ifstream f(std::filesystem::u8path(pathUtf8), std::ios::binary);
        if (!f) return {};
        std::vector<uchar> bytes((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        if (bytes.empty()) return {};
        cv::Mat img = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
        if (img.empty()) return {};
        if (img.channels() == 4) img = flattenAlpha(img);
        return img;
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