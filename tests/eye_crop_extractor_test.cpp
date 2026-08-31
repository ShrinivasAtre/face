#include "EyeCropExtractor.hpp"

#include <opencv2/imgproc.hpp>

#include <cmath>
#include <iostream>

namespace
{
EyeLandmarks eye()
{
    return {{30, 30}, {38, 26}, {52, 26}, {60, 30}, {52, 34}, {38, 34}};
}
bool check(bool value, const char* message)
{
    if (!value) std::cerr << "FAILED: " << message << '\n';
    return value;
}
}

int main()
{
    cv::Mat frame(80, 100, CV_8UC3, cv::Scalar(20, 40, 60));
    cv::line(frame, {30, 30}, {60, 30}, {255, 255, 255}, 2);
    cv::Mat crop;
    if (!check(extractAlignedEyeCrop(frame, eye(), false, crop), "extract") ||
        !check(crop.size() == cv::Size(128, 80), "fixed output") ||
        !check(crop.type() == frame.type(), "type preserved")) return 1;
    auto invalid = eye(); invalid.innerCorner.x = std::nanf("");
    if (!check(!extractAlignedEyeCrop(frame, invalid, false, crop), "invalid rejected") ||
        !check(crop.empty(), "invalid clears output")) return 1;
    if (!check(extractAlignedEyeCrop(frame, eye(), true, crop), "mirrored extract")) return 1;
    return 0;
}
