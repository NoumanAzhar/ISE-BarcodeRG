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

    fs::remove(tmp);
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}