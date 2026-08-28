#include "PfldEyeLandmarkMapper.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
bool check(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}  // namespace

int main()
{
    std::vector<FaceLandmark> source(68);
    for (std::size_t i = 0; i < source.size(); ++i)
        source[i] = {static_cast<float>(i), static_cast<float>(i + 100), 0.0f};

    SemanticEyeLandmarks result;
    if (!check(mapPfldEyeLandmarks(source, result), "valid topology should map") ||
        !check(result.rightEye.outerCorner.x == 36.0f, "right outer index") ||
        !check(result.rightEye.innerCorner.x == 39.0f, "right inner index") ||
        !check(result.leftEye.outerCorner.x == 45.0f, "left outer index") ||
        !check(result.leftEye.innerCorner.x == 42.0f, "left inner index"))
        return 1;

    std::vector<FaceLandmark> shortSource(47);
    if (!check(!mapPfldEyeLandmarks(shortSource, result), "short topology rejected"))
        return 1;

    source[38].x = std::numeric_limits<float>::quiet_NaN();
    if (!check(!mapPfldEyeLandmarks(source, result), "non-finite point rejected") ||
        !check(result.rightEye.outerCorner == cv::Point2f(), "failure resets output"))
        return 1;

    return 0;
}
