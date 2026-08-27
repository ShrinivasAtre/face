#include "BackendGazeMapper.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
cv::Point2f point(const FaceLandmark &value)
{
    return {value.x, value.y};
}

cv::Point2f average(const std::vector<FaceLandmark> &source, const std::array<std::size_t, 5> &indices)
{
    cv::Point2f sum;
    for (const auto index : indices)
        sum += point(source[index]);
    return sum * 0.2F;
}

bool normalizedEye(const cv::Point2f &iris, const cv::Point2f &outer, const cv::Point2f &inner,
                   const cv::Point2f &upper, const cv::Point2f &lower, cv::Point2f &result)
{
    const cv::Point2f horizontal = inner - outer;
    const float width = static_cast<float>(cv::norm(horizontal));
    const float height = static_cast<float>(cv::norm(lower - upper));
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 1.0F || height <= 0.5F)
        return false;
    const cv::Point2f center = (outer + inner) * 0.5F;
    const cv::Point2f verticalCenter = (upper + lower) * 0.5F;
    result.x = 2.0F * (iris.x - center.x) / width;
    result.y = 2.0F * (iris.y - verticalCenter.y) / height;
    return std::isfinite(result.x) && std::isfinite(result.y);
}
} // namespace

bool mapBackendGaze(BackendKind backend, const FaceResult &source, SemanticGaze &result) noexcept
{
    result = {};
    if (backend != BackendKind::MediaPipe || !source.detected || !source.landmarksValid ||
        source.landmarks.size() < 478)
        return false;

    const cv::Point2f rightIris = average(source.landmarks, {468, 469, 470, 471, 472});
    const cv::Point2f leftIris = average(source.landmarks, {473, 474, 475, 476, 477});
    cv::Point2f right, left;
    if (!normalizedEye(rightIris, point(source.landmarks[33]), point(source.landmarks[133]),
                       point(source.landmarks[159]), point(source.landmarks[145]), right) ||
        !normalizedEye(leftIris, point(source.landmarks[263]), point(source.landmarks[362]),
                       point(source.landmarks[386]), point(source.landmarks[374]), left))
        return false;

    // Camera image x is opposite the driver's anatomical left/right convention.
    result.horizontal = std::clamp(-0.5F * (right.x + left.x), -1.0F, 1.0F);
    result.vertical = std::clamp(0.5F * (right.y + left.y), -1.0F, 1.0F);
    const float difference = static_cast<float>(cv::norm(right - left));
    result.interEyeAgreement = std::clamp(1.0F - difference, 0.0F, 1.0F);
    return std::isfinite(result.horizontal) && std::isfinite(result.vertical);
}
