#include "barcode_reader.h"
#include "image_prep.h"       // Member 3: extractRuns
#include "barcode_decode.h"   // Member 4: decodeRuns
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
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

    // Tries the image as it is, upside down and at other scales.
    std::optional<std::string> decodeImage(const cv::Mat& img) {
        // Attempt 1: image as it is
        if (auto r = tryDecode(img)) return r;

        // Attempt 2: upside-down barcode
        cv::Mat flipped;
        cv::rotate(img, flipped, cv::ROTATE_180);
        if (auto r = tryDecode(flipped)) return r;

        // Attempt 3: other scales (tiny images enlarged, huge ones shrunk)
        for (double s : {2.0, 0.5}) {
            if (img.cols * s > 4000 || img.cols * s < 100) continue;
            cv::Mat scaled;
            cv::resize(img, scaled, cv::Size(), s, s, s > 1 ? cv::INTER_LINEAR : cv::INTER_AREA);
            if (auto r = tryDecode(scaled)) return r;
        }
        return std::nullopt;   // never guess: a wrong number is worse than "not found"
    }

} // namespace

std::optional<std::string> readBarcodeFile(const std::string& path) {
    cv::Mat img = loadImage(path);
    if (img.empty()) return std::nullopt;
    return decodeImage(img);
}

std::optional<std::string> readBarcodeFrame(const cv::Mat& frame) {
    if (frame.empty()) return std::nullopt;

    // 1. The whole frame (works when the bar code fills the picture).
    if (auto r = decodeImage(frame)) return r;

    // 2. Camera pictures: extractRuns() only looks at the middle row, so cut
    //    thin strips at several heights (each strip's middle row is the line
    //    we want to scan). Averaging the strip's rows removes camera noise.
    //    Narrower centred crops help when there is background at the sides.
    cv::Mat gray;
    if (frame.channels() == 3)      cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    else if (frame.channels() == 4) cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
    else                            gray = frame;

    for (double widthFrac : {1.0, 0.7, 0.5}) {
        const int w = std::max(1, int(gray.cols * widthFrac));
        const int x0 = (gray.cols - w) / 2;
        for (double heightFrac : {0.5, 0.4, 0.6, 0.3, 0.7, 0.2, 0.8}) {
            const int y = int(gray.rows * heightFrac);
            const int y0 = std::max(0, y - 3);
            const int y1 = std::min(gray.rows, y + 4);
            if (y1 <= y0) continue;

            cv::Mat avg, strip;
            cv::reduce(gray(cv::Range(y0, y1), cv::Range(x0, x0 + w)), avg, 0,
                       cv::REDUCE_AVG, CV_8U);
            cv::repeat(avg, 3, 1, strip);       // 3 rows -> middle row is row 1

            if (auto r = tryDecode(strip)) return r;
            cv::Mat flipped;
            cv::rotate(strip, flipped, cv::ROTATE_180);
            if (auto r = tryDecode(flipped)) return r;
        }
    }
    return std::nullopt;
}
