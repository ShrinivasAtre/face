#pragma once

#include <opencv2/core.hpp>

// Six semantic points for one subject eye, ordered for the standard EAR
// calculation: vertical pairs (upperOuterLid, lowerOuterLid) and
// (upperInnerLid, lowerInnerLid), horizontal pair (outerCorner, innerCorner).
struct EyeLandmarks
{
    cv::Point2f outerCorner;
    cv::Point2f upperOuterLid;
    cv::Point2f upperInnerLid;
    cv::Point2f innerCorner;
    cv::Point2f lowerInnerLid;
    cv::Point2f lowerOuterLid;
};

struct SemanticEyeLandmarks
{
    EyeLandmarks rightEye;
    EyeLandmarks leftEye;
};
