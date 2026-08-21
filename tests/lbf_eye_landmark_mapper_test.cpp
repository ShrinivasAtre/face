#include "LbfEyeLandmarkMapper.hpp"

#include <opencv2/core.hpp>

#include <iostream>
#include <limits>
#include <vector>

namespace
{
bool check(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << std::endl;
    return condition;
}

bool equals(const cv::Point2f& point, float value)
{
    return point.x == value && point.y == value + 0.5f;
}
}  // namespace

int main()
{
    std::vector<cv::Point2f> source(68);
    for (std::size_t i = 0; i < source.size(); ++i)
    {
        source[i] = cv::Point2f(
            static_cast<float>(i),
            static_cast<float>(i) + 0.5f);
    }

    SemanticEyeLandmarks result;
    if (!check(mapLbfEyeLandmarks(source, result), "valid LBF topology") ||
        !check(equals(result.rightEye.outerCorner, 36.0f), "right outer") ||
        !check(equals(result.rightEye.upperOuterLid, 37.0f), "right upper outer") ||
        !check(equals(result.rightEye.upperInnerLid, 38.0f), "right upper inner") ||
        !check(equals(result.rightEye.innerCorner, 39.0f), "right inner") ||
        !check(equals(result.rightEye.lowerInnerLid, 40.0f), "right lower inner") ||
        !check(equals(result.rightEye.lowerOuterLid, 41.0f), "right lower outer") ||
        !check(equals(result.leftEye.outerCorner, 45.0f), "left outer") ||
        !check(equals(result.leftEye.upperOuterLid, 44.0f), "left upper outer") ||
        !check(equals(result.leftEye.upperInnerLid, 43.0f), "left upper inner") ||
        !check(equals(result.leftEye.innerCorner, 42.0f), "left inner") ||
        !check(equals(result.leftEye.lowerInnerLid, 47.0f), "left lower inner") ||
        !check(equals(result.leftEye.lowerOuterLid, 46.0f), "left lower outer"))
    {
        return 1;
    }

    std::vector<cv::Point2f> tooShort(47);
    if (!check(!mapLbfEyeLandmarks(tooShort, result), "short topology should fail"))
        return 1;

    source[43].y = std::numeric_limits<float>::infinity();
    if (!check(!mapLbfEyeLandmarks(source, result), "non-finite point should fail") ||
        !check(result.leftEye.outerCorner == cv::Point2f(), "failure should reset result"))
    {
        return 1;
    }

    std::cout << "LBF eye landmark mapper tests PASSED" << std::endl;
    return 0;
}
