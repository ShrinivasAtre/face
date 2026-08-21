#pragma once

#include "EyeLandmarks.hpp"

#include <opencv2/core.hpp>

#include <vector>

bool mapLbfEyeLandmarks(
    const std::vector<cv::Point2f>& source,
    SemanticEyeLandmarks& result);
