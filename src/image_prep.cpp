#include"image_prep.h"
using namespace std;
vector<int>extractRuns(const cv::Mat& img) {
    if (img.empty()) {
        return {};
    }
    //Converting img to gray 
    cv::Mat gray;
    if (img.channels() == 3)
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    else if (img.channels() == 4)
        cv::cvtColor(img, gray, cv::COLOR_BGRA2GRAY);
    else
        gray = img;

    //Making every pixel black(0) or white(255)
    cv::Mat bars;
    cv::threshold(gray, bars, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    //taking middle row(we can take any row)
    const uchar* row = bars.ptr<uchar>(bars.rows / 2);

    //Finding first and last black pixel
    int first = 0;
    int last = bars.cols - 1;
    while (first < bars.cols && row[first] != 0)
        first++;
    while (last >= 0 && row[last] != 0)
        last--;
    if (first > last)
        return {};

    //counting width of each bar
    vector<int>widths;
    int count = 1;
    for (int x = first; x < last; x++) {
        if (row[x] == row[x + 1])
            count++;
        else {
            widths.push_back(count);
            count = 1;
        }
    }
    widths.push_back(count);
    return widths;
}