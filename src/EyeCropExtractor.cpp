#include "EyeCropExtractor.hpp"

#include <opencv2/imgproc.hpp>

#include <array>
#include <cmath>

namespace
{
std::array<cv::Point2f, 6> points(const EyeLandmarks& eye)
{
    return {eye.outerCorner, eye.upperOuterLid, eye.upperInnerLid,
            eye.innerCorner, eye.lowerInnerLid, eye.lowerOuterLid};
}
}

bool extractAlignedEyeCrop(const cv::Mat& frame, const EyeLandmarks& eye,
                           bool mirrorForSideNormalization, cv::Mat& crop,
                           const EyeCropConfig& config) noexcept
{
    crop.release();
    if (frame.empty() || config.outputSize.width <= 0 || config.outputSize.height <= 0 ||
        !std::isfinite(config.widthScale) || config.widthScale <= 1.0F)
        return false;
    try
    {
        cv::Point2f center{};
        for (const auto& point : points(eye))
        {
            if (!std::isfinite(point.x) || !std::isfinite(point.y)) return false;
            center += point;
        }
        center *= 1.0F / 6.0F;
        const cv::Point2f axis = eye.innerCorner - eye.outerCorner;
        const float eyeWidth = std::sqrt(axis.dot(axis));
        if (eyeWidth < 4.0F) return false;
        const double angle = -std::atan2(static_cast<double>(axis.y),
                                         static_cast<double>(axis.x)) * 180.0 / CV_PI;
        cv::Mat aligned;
        cv::warpAffine(frame, aligned, cv::getRotationMatrix2D(center, angle, 1.0),
                       frame.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
        const int width = std::max(8, static_cast<int>(std::lround(eyeWidth * config.widthScale)));
        const int height = std::max(6, static_cast<int>(std::lround(
            width * static_cast<double>(config.outputSize.height) / config.outputSize.width)));
        cv::Mat roi;
        cv::getRectSubPix(aligned, cv::Size(width, height), center, roi);
        if (roi.empty()) return false;
        cv::resize(roi, crop, config.outputSize, 0.0, 0.0, cv::INTER_AREA);
        if (mirrorForSideNormalization) cv::flip(crop, crop, 1);
        return !crop.empty();
    }
    catch (const cv::Exception&)
    {
        crop.release();
        return false;
    }
}
