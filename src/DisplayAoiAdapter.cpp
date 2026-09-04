#include "DisplayAoiAdapter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
cv::Rect paddedBounds(const std::vector<cv::Point2f> &points, cv::Size size,
                      float horizontalPadding, float verticalPadding) noexcept
{
    if (points.empty() || size.width <= 0 || size.height <= 0) return {};
    float left = std::numeric_limits<float>::max();
    float top = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::lowest();
    float bottom = std::numeric_limits<float>::lowest();
    for (const auto &point : points)
    {
        if (!std::isfinite(point.x) || !std::isfinite(point.y)) return {};
        left = std::min(left, point.x);
        top = std::min(top, point.y);
        right = std::max(right, point.x);
        bottom = std::max(bottom, point.y);
    }
    const float width = std::max(1.0F, right - left);
    const float height = std::max(1.0F, bottom - top);
    const int x0 = static_cast<int>(std::floor(left - width * horizontalPadding));
    const int y0 = static_cast<int>(std::floor(top - height * verticalPadding));
    const int x1 = static_cast<int>(std::ceil(right + width * horizontalPadding));
    const int y1 = static_cast<int>(std::ceil(bottom + height * verticalPadding));
    return cv::Rect(x0, y0, x1 - x0, y1 - y0) & cv::Rect(0, 0, size.width, size.height);
}

void appendEye(std::vector<cv::Point2f> &points, const EyeLandmarks &eye)
{
    points.insert(points.end(), {eye.outerCorner, eye.upperOuterLid,
                                 eye.upperInnerLid, eye.innerCorner,
                                 eye.lowerInnerLid, eye.lowerOuterLid});
}
}

namespace dms
{
DisplayAoi selectDisplayAoi(cv::Size frameSize, DisplayFocus focus,
                            bool faceDetected, const cv::Rect &faceBox,
                            const SemanticEyeLandmarks *eyes,
                            const SemanticFaceGeometry *faceGeometry) noexcept
{
    const cv::Rect frameBounds(0, 0, frameSize.width, frameSize.height);
    if (frameSize.width <= 0 || frameSize.height <= 0) return {};
    if (focus == DisplayFocus::Full) return {frameBounds, true};

    cv::Rect selected;
    if (focus == DisplayFocus::Face && faceDetected)
    {
        selected = faceBox & frameBounds;
    }
    else if (focus == DisplayFocus::Eyes && eyes)
    {
        std::vector<cv::Point2f> points;
        points.reserve(12);
        appendEye(points, eyes->rightEye);
        appendEye(points, eyes->leftEye);
        selected = paddedBounds(points, frameSize, 0.25F, 1.25F);
    }
    else if (focus == DisplayFocus::Mouth && faceGeometry)
    {
        selected = paddedBounds({faceGeometry->rightMouthCorner,
                                 faceGeometry->leftMouthCorner,
                                 faceGeometry->upperInnerLip,
                                 faceGeometry->lowerInnerLip},
                                frameSize, 0.35F, 1.5F);
    }
    if (selected.width <= 0 || selected.height <= 0) return {frameBounds, false};
    return {selected, true};
}
}
