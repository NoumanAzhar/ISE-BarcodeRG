#include "barcode_render.h"

using namespace std;

cv::Mat renderBarcode(const std::string& modules, int moduleW = 3, int height = 100)
{
    if (modules.empty())
        return cv::Mat();
    if (moduleW == 0)
        moduleW = 3;
    else if (moduleW < 0)
        moduleW = -moduleW;
    if (height == 0)
        height = 100;
    else if (height < 0)
        height = -height;
    int xpos = 10 * moduleW;
    for (int i = 0; i < modules.length(); i++)
    {
        if (modules[i] != '0' && modules[i] != '1')
        {
            return cv::Mat();
        }
    }
    cv::Mat img(height + 50, moduleW* modules.length() + moduleW * 20, CV_8UC1, cv::Scalar(255));   // white background
    for (int i = 0; i<modules.length(); i++)
    {
        int d = modules[i] - '0';
        if(d)
        {
            cv::rectangle(img, cv::Rect(xpos, 25, moduleW, height), cv::Scalar(0), cv::FILLED);        // black bar
        }
        xpos += moduleW;
    }
    return img;
}

bool saveBarcode(const cv::Mat& img, const std::string& path)
{
    return cv::imwrite(path, img);
}
