#include "DmsEyeQualityAssessor.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
struct RoiAssessment
{
    float visibility = 0.0F;
    float mean = 0.0F;
    float contrast = 0.0F;
    float laplacianVariance = 0.0F;
};

std::array<cv::Point2f, 6> points(const EyeLandmarks &eye)
{
    return {eye.outerCorner, eye.upperOuterLid, eye.upperInnerLid,
            eye.innerCorner, eye.lowerInnerLid, eye.lowerOuterLid};
}

bool eyeBounds(const EyeLandmarks &eye, float expansion, cv::Rect2f &bounds)
{
    float minX = std::numeric_limits<float>::max(), minY = minX;
    float maxX = std::numeric_limits<float>::lowest(), maxY = maxX;
    for (const auto &point : points(eye))
    {
        if (!std::isfinite(point.x) || !std::isfinite(point.y))
            return false;
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }
    const float width = maxX - minX, height = maxY - minY;
    if (width <= 1.0F || height <= 0.5F)
        return false;
    const float padding = width * expansion;
    bounds = {minX - padding, minY - padding, width + 2 * padding, height + 2 * padding};
    return bounds.area() > 1.0F;
}

bool assessRoi(const cv::Mat &gray, const EyeLandmarks &eye, float expansion, RoiAssessment &result)
{
    cv::Rect2f requested;
    if (!eyeBounds(eye, expansion, requested))
        return false;
    const cv::Rect2f imageBounds(0, 0, static_cast<float>(gray.cols), static_cast<float>(gray.rows));
    const cv::Rect2f clipped = requested & imageBounds;
    result.visibility = requested.area() > 0 ? clipped.area() / requested.area() : 0.0F;
    const cv::Rect roi(static_cast<int>(std::floor(clipped.x)), static_cast<int>(std::floor(clipped.y)),
                       static_cast<int>(std::ceil(clipped.width)), static_cast<int>(std::ceil(clipped.height)));
    const cv::Rect safe = roi & cv::Rect(0, 0, gray.cols, gray.rows);
    if (safe.width < 2 || safe.height < 2)
        return false;
    cv::Scalar mean, deviation;
    cv::meanStdDev(gray(safe), mean, deviation);
    cv::Mat laplacian;
    cv::Laplacian(gray(safe), laplacian, CV_32F);
    cv::Scalar lapMean, lapDeviation;
    cv::meanStdDev(laplacian, lapMean, lapDeviation);
    result.mean = static_cast<float>(mean[0]);
    result.contrast = static_cast<float>(deviation[0]);
    result.laplacianVariance = static_cast<float>(lapDeviation[0] * lapDeviation[0]);
    return true;
}

float scoreAbove(float value, float minimum)
{
    return minimum > 0.0F ? std::clamp(value / minimum, 0.0F, 1.0F) : 1.0F;
}
} // namespace

bool EyeQualityConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (!std::isfinite(roiExpansion) || roiExpansion < 0.0F || !std::isfinite(minimumVisibleFraction) ||
        minimumVisibleFraction < 0.0F || minimumVisibleFraction > 1.0F || !std::isfinite(darkLimit) ||
        !std::isfinite(brightLimit) || darkLimit < 0.0F || brightLimit > 255.0F || darkLimit >= brightLimit ||
        !std::isfinite(minimumContrast) || minimumContrast < 0.0F || !std::isfinite(minimumLaplacianVariance) ||
        minimumLaplacianVariance < 0.0F)
    {
        error = "invalid eye ROI quality thresholds";
        return false;
    }
    return true;
}

EyeQualityAssessor::EyeQualityAssessor(EyeQualityConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}

EyeQualityResult EyeQualityAssessor::assess(const cv::Mat &frame, const SemanticEyeLandmarks &eyes) const noexcept
{
    EyeQualityResult result;
    if (!valid_ || frame.empty())
        return result;
    try
    {
        cv::Mat gray;
        if (frame.channels() == 1)
            gray = frame;
        else if (frame.channels() == 3)
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        else if (frame.channels() == 4)
            cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
        else
            return result;
        RoiAssessment right, left;
        if (!assessRoi(gray, eyes.rightEye, config_.roiExpansion, right) ||
            !assessRoi(gray, eyes.leftEye, config_.roiExpansion, left))
            return result;
        result.visibility = std::min(right.visibility, left.visibility);
        result.meanIntensity = 0.5F * (right.mean + left.mean);
        result.contrast = std::min(right.contrast, left.contrast);
        result.laplacianVariance = std::min(right.laplacianVariance, left.laplacianVariance);
        if (result.visibility < config_.minimumVisibleFraction)
        {
            result.validity = dms::SourceValidity::Occluded;
            return result;
        }
        result.validity = dms::SourceValidity::Valid;
        const float exposureScore =
            result.meanIntensity <= config_.darkLimit
                ? std::clamp(result.meanIntensity / config_.darkLimit, 0.0F, 1.0F)
                : (result.meanIntensity >= config_.brightLimit
                       ? std::clamp((255.0F - result.meanIntensity) / (255.0F - config_.brightLimit), 0.0F, 1.0F)
                       : 1.0F);
        result.confidence = std::min({exposureScore, scoreAbove(result.contrast, config_.minimumContrast),
                                      scoreAbove(result.laplacianVariance, config_.minimumLaplacianVariance)});
    }
    catch (const cv::Exception &)
    {
        return {};
    }
    return result;
}
