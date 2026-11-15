#include "barcode_reader.h"
#include "image_prep.h"       // Member 3: extractRuns
#include "barcode_decode.h"   // Member 4: decodeRuns
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>
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

    // ------------------------------------------------------------------
    // Camera reading
    // ------------------------------------------------------------------

    cv::Mat toGray(const cv::Mat& frame) {
        cv::Mat gray;
        if (frame.channels() == 3)      cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        else if (frame.channels() == 4) cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
        else                            gray = frame;
        if (gray.depth() != CV_8U) gray.convertTo(gray, CV_8U);
        return gray;
    }

    // One scan line: a 1 x W row of gray values (already averaged over a few
    // image rows). extractRuns() thresholds with Otsu on whatever it gets, so
    // we offer it a few versions of the line and try both reading directions.
    std::optional<std::string> readScanLine(const cv::Mat& line) {
        for (double scale : {1.0, 2.0}) {
            // stretch narrow lines so thin bars are not rounded away
            if (scale > 1.0 && line.cols >= 700) continue;
            cv::Mat p;
            if (scale > 1.0) cv::resize(line, p, cv::Size(), scale, 1.0, cv::INTER_CUBIC);
            else             p = line;

            for (int variant = 0; variant < 3; ++variant) {
                cv::Mat q;
                if (variant == 0) {
                    q = p;                                        // as it is
                } else if (variant == 1) {
                    cv::GaussianBlur(p, q, cv::Size(5, 1), 0);    // removes 1-pixel noise
                } else {
                    // removes uneven lighting (shadow / glare across the bar code)
                    cv::Mat f, bg, n;
                    p.convertTo(f, CV_32F);
                    const int k = (p.cols / 6) | 1;               // odd kernel
                    cv::GaussianBlur(f, bg, cv::Size(k, 1), 0);
                    cv::subtract(f, bg, n);
                    n.convertTo(q, CV_8U, 1.0, 128.0);            // +128, saturates to 0..255
                }

                cv::Mat strip, flipped;
                cv::repeat(q, 3, 1, strip);                       // 3 rows -> middle row is row 1
                if (auto r = tryDecode(strip)) return r;
                cv::flip(strip, flipped, 1);                      // upside-down bar code
                if (auto r = tryDecode(flipped)) return r;
            }
        }
        return std::nullopt;
    }

    // Cuts a scan line down to the part that contains the bars.
    // Bars make many edges close together; a table edge or the border of the
    // paper makes one lonely edge. So: find the edges, group edges that are
    // close to each other, keep the biggest group, add a little quiet zone.
    // Returns an empty Mat when no group of bar edges is found.
    cv::Mat trimToBars(const cv::Mat& line) {
        const int w = line.cols;
        if (w < 32) return {};

        std::vector<float> edge(size_t(w - 1)), sm(size_t(w - 1));
        for (int i = 0; i + 1 < w; ++i)
            edge[size_t(i)] = std::fabs(float(line.at<uchar>(0, i + 1)) - float(line.at<uchar>(0, i)));
        const int n = int(edge.size());
        for (int i = 0; i < n; ++i)
            sm[size_t(i)] = (edge[size_t(std::max(i - 1, 0))] + edge[size_t(i)]
                             + edge[size_t(std::min(i + 1, n - 1))]) / 3.f;

        const float limit = 0.3f * *std::max_element(sm.begin(), sm.end());
        std::vector<int> peaks;
        for (int i = 1; i + 1 < n; ++i) {
            if (sm[size_t(i)] >= limit && sm[size_t(i)] >= sm[size_t(i - 1)]
                && sm[size_t(i)] >= sm[size_t(i + 1)]) {
                if (!peaks.empty() && i - peaks.back() < 2) continue;   // same edge seen twice
                peaks.push_back(i);
            }
        }
        if (peaks.size() < 20) return {};

        std::vector<int> gaps;
        for (size_t i = 1; i < peaks.size(); ++i) gaps.push_back(peaks[i] - peaks[i - 1]);
        std::vector<int> sortedGaps = gaps;
        std::sort(sortedGaps.begin(), sortedGaps.end());
        const double module = std::max(1.0, double(sortedGaps[size_t(0.2 * (sortedGaps.size() - 1))]));
        const double maxGap = 6.0 * module;       // bars are never further apart than this

        size_t bestCount = 0, bestFirst = 0, bestLast = 0, start = 0;
        for (size_t i = 1; i <= peaks.size(); ++i) {
            if (i == peaks.size() || peaks[i] - peaks[i - 1] > maxGap) {
                if (i - start > bestCount) { bestCount = i - start; bestFirst = start; bestLast = i - 1; }
                start = i;
            }
        }
        if (bestCount < 20) return {};

        const int pad = int(std::max(6.0, 3.0 * module));
        const int x0 = std::max(0, peaks[bestFirst] - pad);
        const int x1 = std::min(w, peaks[bestLast] + pad + 2);
        if (x1 - x0 < 20) return {};
        return line.colRange(x0, x1).clone();
    }

    // Scan lines at several heights of a (straightened) crop.
    std::optional<std::string> readCrop(const cv::Mat& crop) {
        const int h = crop.rows;
        const int half = std::min(4, std::max(1, h / 30));
        for (double f : {0.5, 0.35, 0.65, 0.25, 0.75}) {
            const int y = int(h * f);
            const int y0 = std::max(0, y - half);
            const int y1 = std::min(h, y + half + 1);
            if (y1 <= y0) continue;
            cv::Mat avg;
            cv::reduce(crop.rowRange(y0, y1), avg, 0, cv::REDUCE_AVG, CV_8U);
            const cv::Mat bars = trimToBars(avg);
            if (bars.empty()) continue;
            if (auto r = readScanLine(bars)) return r;
        }
        return std::nullopt;
    }

    // A candidate area of the frame that may hold a bar code.
    struct Region {
        cv::Point2f center;     // frame coordinates
        float length = 0;       // long side = reading direction
        float thickness = 0;    // short side = bar height
        cv::Rect box;           // axis-aligned box, used for drawing
    };

    // Finds areas with many vertical edges: the frame is cut into 8x8 pixel
    // cells, a cell counts as "bars" when its left-right edge strength is
    // clearly bigger than its up-down edge strength, and touching cells are
    // joined into regions. The best 3 regions (by total edge strength) are kept.
    std::vector<Region> findBarcodeRegions(const cv::Mat& gray) {
        // Work on at most 640 px width.
        const double sc = gray.cols > 640 ? 640.0 / gray.cols : 1.0;
        cv::Mat g;
        if (sc < 1.0) cv::resize(gray, g, cv::Size(), sc, sc, cv::INTER_AREA);
        else          g = gray;

        constexpr int B = 8;                                  // cell size in pixels
        const cv::Size grid(g.cols / B, g.rows / B);
        if (grid.width < 8 || grid.height < 4) return {};

        cv::Mat sx, sy;
        cv::Scharr(g, sx, CV_32F, 1, 0);
        cv::Scharr(g, sy, CV_32F, 0, 1);
        const cv::Mat ax = cv::abs(sx);
        const cv::Mat ay = cv::abs(sy);

        // average edge strength per cell
        cv::Mat mx, my;
        cv::resize(ax, mx, grid, 0, 0, cv::INTER_AREA);
        cv::resize(ay, my, grid, 0, 0, cv::INTER_AREA);

        // reference strength: the 99th percentile of the cells
        std::vector<float> vals(mx.begin<float>(), mx.end<float>());
        const size_t k99 = size_t(vals.size() * 0.99);
        std::nth_element(vals.begin(), vals.begin() + k99, vals.end());
        const double strong = vals[k99];

        cv::Mat vertLimit, horizOk, strongOk, cells;
        cv::addWeighted(my, 1.8, my, 0.0, 1.0, vertLimit);    // 1.8 * up-down + 1
        cv::compare(mx, vertLimit, horizOk, cv::CMP_GT);
        cv::compare(mx, 0.25 * strong, strongOk, cv::CMP_GT);
        cv::bitwise_and(horizOk, strongOk, cells);

        // close small holes between the bars of the same code
        cv::morphologyEx(cells, cells, cv::MORPH_CLOSE,
                         cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 3)));

        cv::Mat labels, stats, centroids;
        const int count = cv::connectedComponentsWithStats(cells, labels, stats, centroids, 8);

        std::vector<std::pair<double, int>> order;           // (total edge strength, label)
        for (int i = 1; i < count; ++i) {
            const int area = stats.at<int>(i, cv::CC_STAT_AREA);
            if (area < 12) continue;
            const cv::Rect r(stats.at<int>(i, cv::CC_STAT_LEFT), stats.at<int>(i, cv::CC_STAT_TOP),
                             stats.at<int>(i, cv::CC_STAT_WIDTH), stats.at<int>(i, cv::CC_STAT_HEIGHT));
            const cv::Mat inRegion = labels(r) == i;
            order.emplace_back(cv::mean(mx(r), inRegion)[0] * area, i);
        }
        std::sort(order.begin(), order.end(),
                  [](const std::pair<double, int>& a, const std::pair<double, int>& b) {
                      return a.first > b.first;
                  });
        if (order.size() > 3) order.resize(3);

        const float inv = float(1.0 / sc);
        const cv::Rect bounds(0, 0, gray.cols, gray.rows);
        std::vector<Region> regions;
        for (const auto& o : order) {
            const cv::Rect r(stats.at<int>(o.second, cv::CC_STAT_LEFT), stats.at<int>(o.second, cv::CC_STAT_TOP),
                             stats.at<int>(o.second, cv::CC_STAT_WIDTH), stats.at<int>(o.second, cv::CC_STAT_HEIGHT));
            std::vector<cv::Point> cellPoints;
            cv::findNonZero(labels(r) == o.second, cellPoints);
            std::vector<cv::Point2f> pts;
            for (const cv::Point& p : cellPoints)
                pts.emplace_back(float((p.x + r.x) * B + B / 2), float((p.y + r.y) * B + B / 2));
            if (pts.size() < 3) continue;

            const cv::RotatedRect rr = cv::minAreaRect(pts);
            const float w = rr.size.width + B;                // cells are coarse: add one cell
            const float h = rr.size.height + B;
            const float longSide = std::max(w, h);
            const float shortSide = std::min(w, h);
            if (longSide < 30.f || shortSide < 12.f) continue;

            const cv::Rect bb = rr.boundingRect();
            Region reg;
            reg.center = cv::Point2f(rr.center.x * inv, rr.center.y * inv);
            reg.length = longSide * inv;
            reg.thickness = shortSide * inv;
            reg.box = cv::Rect(int(bb.x * inv), int(bb.y * inv),
                               int(bb.width * inv), int(bb.height * inv)) & bounds;
            regions.push_back(reg);
        }
        return regions;
    }

    // Brightness of a rotated copy of the window along the middle band:
    // the sum of all left-right changes of the band's average line. When the
    // bars are exactly vertical the rows reinforce each other, so this is largest.
    double lineContrast(const cv::Mat& win, cv::Point2f c, double angle, float length, float thickness) {
        cv::Mat rot;
        cv::warpAffine(win, rot, cv::getRotationMatrix2D(c, angle, 1.0), win.size(),
                       cv::INTER_LINEAR, cv::BORDER_REPLICATE);
        const int y0 = std::max(0, int(c.y - thickness / 4));
        const int y1 = std::min(rot.rows, int(c.y + thickness / 4) + 1);
        const int x0 = std::max(0, int(c.x - length / 2));
        const int x1 = std::min(rot.cols, int(c.x + length / 2));
        if (y1 <= y0 || x1 - x0 < 3) return 0.0;

        cv::Mat line;
        cv::reduce(rot(cv::Range(y0, y1), cv::Range(x0, x1)), line, 0, cv::REDUCE_AVG, CV_32F);
        double sum = 0.0;
        for (int i = 0; i + 1 < line.cols; ++i)
            sum += std::fabs(line.at<float>(0, i + 1) - line.at<float>(0, i));
        return sum;
    }

    // Tilt of the bar code in degrees: try angles and keep the one that makes
    // the bars sharpest (coarse steps first, then fine steps around the best).
    double estimateTilt(const cv::Mat& win, cv::Point2f c, float length, float thickness) {
        // a half-size copy is enough for this and 4 times faster
        const double k = length > 300.f ? 0.5 : 1.0;
        cv::Mat small;
        if (k < 1.0) cv::resize(win, small, cv::Size(), k, k, cv::INTER_AREA);
        else         small = win;
        const cv::Point2f cs(float(c.x * k), float(c.y * k));
        const float ls = float(length * k), ts = float(thickness * k);

        double bestAngle = 0.0;
        double bestScore = lineContrast(small, cs, 0.0, ls, ts);
        for (int a = -24; a <= 24; a += 4) {
            const double v = lineContrast(small, cs, a, ls, ts);
            if (v > bestScore) { bestScore = v; bestAngle = a; }
        }
        const double coarse = bestAngle;
        for (int d : {-3, -2, -1, 1, 2, 3}) {
            const double v = lineContrast(small, cs, coarse + d, ls, ts);
            if (v > bestScore) { bestScore = v; bestAngle = coarse + d; }
        }
        return bestAngle;
    }

    // Finds regions in `gray`, straightens each one and tries to read it.
    // `turned90` = `gray` is the frame rotated 90 deg clockwise (bar code that
    // stands upright); the reported box is mapped back to the original frame.
    bool readRegions(const cv::Mat& gray, bool turned90, CameraScanResult& out) {
        const cv::Rect bounds(0, 0, gray.cols, gray.rows);

        for (const Region& reg : findBarcodeRegions(gray)) {
            if (out.box.area() == 0) {
                out.box = turned90
                    ? cv::Rect(reg.box.y, gray.cols - reg.box.x - reg.box.width, reg.box.height, reg.box.width)
                    : reg.box;
            }

            // work on a window around the region (cheaper than the whole frame)
            const float radius = std::hypot(reg.length / 2 + reg.length * 0.12f + 20.f, reg.thickness / 2) + 4.f;
            cv::Rect roi(int(reg.center.x - radius), int(reg.center.y - radius),
                         int(2 * radius), int(2 * radius));
            roi &= bounds;
            if (roi.width < 20 || roi.height < 5) continue;
            const cv::Mat win = gray(roi);
            const cv::Point2f c(reg.center.x - roi.x, reg.center.y - roi.y);

            const double tilt = estimateTilt(win, c, reg.length, reg.thickness);
            std::vector<double> angles;
            if (std::fabs(tilt) > 0.5) angles = { tilt, 0.0 };
            else                       angles = { 0.0 };

            for (double ang : angles) {
                cv::Mat rot;
                cv::warpAffine(win, rot, cv::getRotationMatrix2D(c, ang, 1.0), win.size(),
                               cv::INTER_LINEAR, cv::BORDER_REPLICATE);

                // rows of the region (now level), some room left and right; the
                // exact left/right ends are found per scan line by trimToBars()
                for (double padFrac : {0.12, 0.04}) {
                    const double pad = std::max(20.0, padFrac * reg.length);
                    const int x0 = int(std::max(0.0, c.x - reg.length / 2 - pad));
                    const int x1 = int(std::min<double>(rot.cols, c.x + reg.length / 2 + pad));
                    const int y0 = int(std::max(0.0, c.y - reg.thickness / 2));
                    const int y1 = int(std::min<double>(rot.rows, c.y + reg.thickness / 2));
                    if (x1 - x0 < 20 || y1 - y0 < 5) continue;
                    if (auto r = readCrop(rot(cv::Range(y0, y1), cv::Range(x0, x1)))) {
                        out.value = r;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Last resort when nothing was located: thin strips across the frame.
    std::optional<std::string> sweepStrips(const cv::Mat& frame, const cv::Mat& gray) {
        if (auto r = decodeImage(frame)) return r;   // bar code fills the picture

        for (double widthFrac : {1.0, 0.7, 0.5}) {
            const int w = std::max(1, int(gray.cols * widthFrac));
            const int x0 = (gray.cols - w) / 2;
            for (double heightFrac : {0.5, 0.4, 0.6, 0.3, 0.7, 0.2, 0.8}) {
                const int y = int(gray.rows * heightFrac);
                const int y0 = std::max(0, y - 3);
                const int y1 = std::min(gray.rows, y + 4);
                if (y1 <= y0) continue;
                cv::Mat avg;
                cv::reduce(gray(cv::Range(y0, y1), cv::Range(x0, x0 + w)), avg, 0,
                           cv::REDUCE_AVG, CV_8U);
                const cv::Mat bars = trimToBars(avg);
                if (!bars.empty())
                    if (auto r = readScanLine(bars)) return r;
                if (auto r = readScanLine(avg)) return r;
            }
        }
        return std::nullopt;
    }

} // namespace

std::optional<std::string> readBarcodeFile(const std::string& path) {
    cv::Mat img = loadImage(path);
    if (img.empty()) return std::nullopt;
    return decodeImage(img);
}

CameraScanResult scanCameraFrame(const cv::Mat& frame) {
    CameraScanResult out;
    if (frame.empty()) return out;

    const cv::Mat gray = toGray(frame);

    // 1. bar code lying flat (normal case, any tilt)
    if (readRegions(gray, false, out)) return out;

    // 2. bar code standing upright: turn the frame and look again
    cv::Mat turned;
    cv::rotate(gray, turned, cv::ROTATE_90_CLOCKWISE);
    if (readRegions(turned, true, out)) return out;

    // 3. nothing located / readable: sweep strips across the frame
    out.value = sweepStrips(frame, gray);
    return out;
}

std::optional<std::string> readBarcodeFrame(const cv::Mat& frame) {
    return scanCameraFrame(frame).value;
}
