#include "DmsEyeQualityAssessor.hpp"

#include <opencv2/imgproc.hpp>

#include <iostream>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
EyeLandmarks eye(float centerX)
{
    return {{centerX - 12, 30}, {centerX - 6, 25}, {centerX + 6, 25},
            {centerX + 12, 30}, {centerX + 6, 35}, {centerX - 6, 35}};
}
} // namespace

int main()
{
    EyeQualityAssessor assessor;
    if (!check(assessor.valid(), "valid defaults"))
        return 1;
    SemanticEyeLandmarks eyes{eye(30), eye(70)};
    cv::Mat textured(60, 100, CV_8UC1);
    for (int row = 0; row < textured.rows; ++row)
        for (int col = 0; col < textured.cols; ++col)
            textured.at<unsigned char>(row, col) = ((row / 2 + col / 2) % 2) ? 220 : 30;
    auto quality = assessor.assess(textured, eyes);
    if (!check(quality.validity == dms::SourceValidity::Valid, "visible eye ROI") ||
        !check(quality.confidence > 0.9F, "textured ROI quality"))
        return 1;
    cv::Mat flat(60, 100, CV_8UC1, cv::Scalar(100));
    quality = assessor.assess(flat, eyes);
    if (!check(quality.validity == dms::SourceValidity::Valid, "flat is in frame") ||
        !check(quality.confidence < 0.1F, "flat ROI low confidence"))
        return 1;
    eyes.leftEye = eye(99);
    quality = assessor.assess(textured, eyes);
    if (!check(quality.validity == dms::SourceValidity::Occluded, "partial eye ROI is occluded"))
        return 1;
    if (!check(assessor.assess({}, eyes).validity == dms::SourceValidity::Missing, "missing frame"))
        return 1;
    std::cout << "DMS eye quality assessor tests PASSED\n";
    return 0;
}
