#pragma once

#include <opencv2/core.hpp>
#include <opencv2/face.hpp>

#include <string>
#include <vector>

class LbfLandmarkDetector
{
public:
    LbfLandmarkDetector() = default;

    explicit LbfLandmarkDetector(
        const std::string& modelPath);

    bool initialize(
        const std::string& modelPath);

    bool detect(
        const cv::Mat& frame,
        const cv::Rect& faceBox,
        std::vector<cv::Point2f>& landmarks);

private:
    cv::Ptr<cv::face::Facemark> facemark_;
};
