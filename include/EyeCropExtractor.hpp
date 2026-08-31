#pragma once

#include "EyeLandmarks.hpp"

#include <opencv2/core.hpp>

struct EyeCropConfig
{
    cv::Size outputSize{128, 80};
    float widthScale = 1.8F;
};

bool extractAlignedEyeCrop(const cv::Mat& frame, const EyeLandmarks& eye,
                           bool mirrorForSideNormalization, cv::Mat& crop,
                           const EyeCropConfig& config = {}) noexcept;
