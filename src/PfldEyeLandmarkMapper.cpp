#include "PfldEyeLandmarkMapper.hpp"

#include <cmath>

namespace
{
cv::Point2f point(const FaceLandmark& source)
{
    return {source.x, source.y};
}

bool finite(const EyeLandmarks& eye)
{
    const cv::Point2f points[] = {
        eye.outerCorner, eye.upperOuterLid, eye.upperInnerLid,
        eye.innerCorner, eye.lowerInnerLid, eye.lowerOuterLid};
    for (const cv::Point2f& value : points)
    {
        if (!std::isfinite(value.x) || !std::isfinite(value.y))
            return false;
    }
    return true;
}
}  // namespace

bool mapPfldEyeLandmarks(
    const std::vector<FaceLandmark>& source,
    SemanticEyeLandmarks& result)
{
    result = {};
    if (source.size() < 48)
        return false;

    // Candidate-specific 68-point iBUG topology. These indices must remain in
    // this adapter even though the current LBF baseline uses the same layout.
    result.rightEye = {
        point(source[36]), point(source[37]), point(source[38]),
        point(source[39]), point(source[40]), point(source[41])};
    result.leftEye = {
        point(source[45]), point(source[44]), point(source[43]),
        point(source[42]), point(source[47]), point(source[46])};

    if (!finite(result.rightEye) || !finite(result.leftEye))
    {
        result = {};
        return false;
    }
    return true;
}

