#include "LbfEyeLandmarkMapper.hpp"

#include <cmath>

namespace
{
bool finite(const cv::Point2f& point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
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

bool mapLbfEyeLandmarks(
    const std::vector<cv::Point2f>& source,
    SemanticEyeLandmarks& result)
{
    result = {};

    if (source.size() < 48)
        return false;

    // Standard 68-point LBF topology. Backend indices are confined here.
    result.rightEye = {
        source[36], source[37], source[38],
        source[39], source[40], source[41]};

    result.leftEye = {
        source[45], source[44], source[43],
        source[42], source[47], source[46]};

    if (!finite(result.rightEye) || !finite(result.leftEye))
    {
        result = {};
        return false;
    }

    return true;
}
