#include "MediaPipeEyeLandmarkMapper.hpp"

#include <cmath>

namespace
{
constexpr std::size_t kMediaPipeLandmarkCount = 478;

cv::Point2f point(const FaceLandmark& landmark)
{
    return {landmark.x, landmark.y};
}

bool finite(const cv::Point2f& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool finite(const EyeLandmarks& eye)
{
    return finite(eye.outerCorner) &&
           finite(eye.upperOuterLid) &&
           finite(eye.upperInnerLid) &&
           finite(eye.innerCorner) &&
           finite(eye.lowerInnerLid) &&
           finite(eye.lowerOuterLid);
}
}  // namespace

bool mapMediaPipeEyeLandmarks(
    const std::vector<FaceLandmark>& source,
    SemanticEyeLandmarks& result)
{
    result = {};

    if (source.size() < kMediaPipeLandmarkCount)
        return false;

    // MediaPipe Face Landmarker 478-point topology. Indices are ordered by
    // semantic position from the subject's outer corner to inner corner.
    result.rightEye = {
        point(source[33]),
        point(source[160]),
        point(source[158]),
        point(source[133]),
        point(source[153]),
        point(source[144])};

    result.leftEye = {
        point(source[263]),
        point(source[387]),
        point(source[385]),
        point(source[362]),
        point(source[380]),
        point(source[373])};

    if (!finite(result.rightEye) || !finite(result.leftEye))
    {
        result = {};
        return false;
    }

    return true;
}
