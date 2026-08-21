#include "MediaPipeEyeLandmarkMapper.hpp"

#include <opencv2/core.hpp>

#include <cmath>
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

float eyeAspectRatio(const EyeLandmarks& eye)
{
    const float verticalOuter = static_cast<float>(
        cv::norm(eye.upperOuterLid - eye.lowerOuterLid));
    const float verticalInner = static_cast<float>(
        cv::norm(eye.upperInnerLid - eye.lowerInnerLid));
    const float horizontal = static_cast<float>(
        cv::norm(eye.outerCorner - eye.innerCorner));
    return (verticalOuter + verticalInner) / (2.0f * horizontal);
}

void setEye(
    std::vector<FaceLandmark>& source,
    const std::size_t indices[6])
{
    const cv::Point2f points[6] = {
        {0.0f, 0.0f}, {1.0f, -1.0f}, {3.0f, -1.0f},
        {4.0f, 0.0f}, {3.0f, 1.0f}, {1.0f, 1.0f}};

    for (std::size_t i = 0; i < 6; ++i)
    {
        source[indices[i]].x = points[i].x;
        source[indices[i]].y = points[i].y;
    }
}
}  // namespace

int main()
{
    std::vector<FaceLandmark> source(478);
    for (std::size_t i = 0; i < source.size(); ++i)
    {
        source[i].x = static_cast<float>(i);
        source[i].y = static_cast<float>(i) + 0.5f;
        source[i].z = std::numeric_limits<float>::quiet_NaN();
    }

    SemanticEyeLandmarks result;
    if (!check(mapMediaPipeEyeLandmarks(source, result), "valid topology") ||
        !check(equals(result.rightEye.outerCorner, 33.0f), "right outer") ||
        !check(equals(result.rightEye.upperOuterLid, 160.0f), "right upper outer") ||
        !check(equals(result.rightEye.upperInnerLid, 158.0f), "right upper inner") ||
        !check(equals(result.rightEye.innerCorner, 133.0f), "right inner") ||
        !check(equals(result.rightEye.lowerInnerLid, 153.0f), "right lower inner") ||
        !check(equals(result.rightEye.lowerOuterLid, 144.0f), "right lower outer") ||
        !check(equals(result.leftEye.outerCorner, 263.0f), "left outer") ||
        !check(equals(result.leftEye.upperOuterLid, 387.0f), "left upper outer") ||
        !check(equals(result.leftEye.upperInnerLid, 385.0f), "left upper inner") ||
        !check(equals(result.leftEye.innerCorner, 362.0f), "left inner") ||
        !check(equals(result.leftEye.lowerInnerLid, 380.0f), "left lower inner") ||
        !check(equals(result.leftEye.lowerOuterLid, 373.0f), "left lower outer"))
    {
        return 1;
    }

    std::vector<FaceLandmark> tooShort(477);
    if (!check(!mapMediaPipeEyeLandmarks(tooShort, result),
               "short topology should fail") ||
        !check(result.rightEye.outerCorner == cv::Point2f(),
               "short topology should reset result"))
    {
        return 1;
    }

    source[0].x = std::numeric_limits<float>::infinity();
    if (!check(mapMediaPipeEyeLandmarks(source, result),
               "unselected coordinate should be ignored"))
    {
        return 1;
    }

    source[385].y = std::numeric_limits<float>::infinity();
    if (!check(!mapMediaPipeEyeLandmarks(source, result),
               "selected non-finite coordinate should fail") ||
        !check(result.leftEye.outerCorner == cv::Point2f(),
               "non-finite failure should reset result"))
    {
        return 1;
    }

    source.assign(478, {});
    const std::size_t rightIndices[6] = {33, 160, 158, 133, 153, 144};
    const std::size_t leftIndices[6] = {263, 387, 385, 362, 380, 373};
    setEye(source, rightIndices);
    setEye(source, leftIndices);

    if (!check(mapMediaPipeEyeLandmarks(source, result), "EAR topology") ||
        !check(std::abs(eyeAspectRatio(result.rightEye) - 0.5f) < 0.0001f,
               "right EAR compatibility") ||
        !check(std::abs(eyeAspectRatio(result.leftEye) - 0.5f) < 0.0001f,
               "left EAR compatibility"))
    {
        return 1;
    }

    std::cout << "MediaPipe eye landmark mapper tests PASSED" << std::endl;
    return 0;
}
