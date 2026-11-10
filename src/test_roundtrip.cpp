#include "code128.h"
#include "barcode_render.h"
#include "barcode_reader.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

static int passed = 0, failed = 0;
static void check(bool ok, const std::string& name) {
    if (ok) ++passed; else { ++failed; std::cout << "FAIL: " << name << "\n"; }
}

static std::string readImg(const cv::Mat& img, const std::string& tmp) {
    saveBarcode(img, tmp);
    auto r = readBarcodeFile(tmp);
    return r ? *r : "";
}

int main(int argc, char** argv) {
    namespace fs = std::filesystem;
    fs::path dir = argc > 1 ? argv[1] : "test_images";
    fs::create_directories(dir);
    std::string tmp = (dir / "_tmp.png").string();

    // 1. Known vector
    std::string m = encodeCode128C("123456");
    check(m.size() == 68, "length 68");
    check(m.rfind("11010011100", 0) == 0, "starts with Start C");
    check(m.substr(m.size() - 13) == "1100011101011", "ends with Stop");

    // 2. Validation
    for (auto s : { "012345", "12345", "1234567", "12a456", "" })
        check(!validate6Digits(s), std::string("reject ") + s);
    for (auto s : { "100000", "999999", "123456" })
        check(validate6Digits(s), std::string("accept ") + s);

    // 3. Random round trips at module widths 2..5
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> first(1, 9), other(0, 9), mw(2, 5);
    for (int i = 0; i < 100; ++i) {
        std::string d(1, char('0' + first(rng)));
        for (int k = 0; k < 5; ++k) d += char('0' + other(rng));
        cv::Mat img = renderBarcode(encodeCode128C(d), mw(rng), 100);
        check(readImg(img, tmp) == d, "roundtrip " + d);
    }

    // 4. Variants of one barcode (also saved as sample test images)
    const std::string D = "123456";
    cv::Mat base = renderBarcode(encodeCode128C(D), 3, 100);
    auto variant = [&](const std::string& name, const cv::Mat& img, bool expectRead) {
        cv::imwrite((dir / (name + ".png")).string(), img);
        auto r = readBarcodeFile((dir / (name + ".png")).string());
        check(expectRead ? (r && *r == D) : !r, name);
        };
    variant("clean", base, true);

    cv::Mat up, down, flip, blur, noisy, border;
    cv::resize(base, up, cv::Size(), 2, 2, cv::INTER_NEAREST);
    cv::resize(base, down, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
    cv::rotate(base, flip, cv::ROTATE_180);
    cv::GaussianBlur(base, blur, cv::Size(3, 3), 0);
    cv::Mat n(base.size(), CV_8SC1);
    cv::randn(n, 0, 12);
    cv::add(base, n, noisy, cv::noArray(), CV_8U);
    cv::copyMakeBorder(base, border, 60, 60, 80, 80, cv::BORDER_CONSTANT, 255);
    variant("scaled_2x", up, true);
    variant("scaled_half", down, true);
    variant("upside_down", flip, true);
    variant("blurred", blur, true);
    variant("noisy", noisy, true);
    variant("big_border", border, true);

    // 5. Must NOT be read
    variant("blank", cv::Mat(100, 300, CV_8UC1, cv::Scalar(255)), false);
    cv::Mat text(100, 300, CV_8UC1, cv::Scalar(255));
    cv::putText(text, "HELLO", { 10, 60 }, cv::FONT_HERSHEY_SIMPLEX, 1.5, 0, 3);
    variant("text_only", text, false);

    { std::ofstream f(dir / "fake.png"); f << "not an image"; }
    check(!readBarcodeFile((dir / "fake.png").string()), "fake png");
    check(!readBarcodeFile("does/not/exist.png"), "missing file");
    check(!readBarcodeFile(""), "empty path");

    fs::remove(tmp);
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}