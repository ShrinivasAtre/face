#include "DmsHeadPoseEstimator.hpp"

#include <opencv2/calib3d.hpp>

#include <array>
#include <cmath>
#include <vector>

namespace
{
const std::array<cv::Point3d, 6> kGenericFaceModel = {
    cv::Point3d{0.0, 0.0, 0.0},         cv::Point3d{0.0, 330.0, -65.0},     cv::Point3d{-225.0, -170.0, -135.0},
    cv::Point3d{225.0, -170.0, -135.0}, cv::Point3d{-150.0, 150.0, -125.0}, cv::Point3d{150.0, 150.0, -125.0}};

bool finite(const cv::Point2f &point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}
} // namespace

bool HeadPoseNeutralConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (confirmation <= dms::MonotonicTime::zero() || !std::isfinite(maximumYawRange) ||
        !std::isfinite(maximumPitchRange) || maximumYawRange <= 0.0F || maximumPitchRange <= 0.0F)
    {
        error = "invalid neutral pose calibration configuration";
        return false;
    }
    return true;
}

HeadPoseNeutralCalibrator::HeadPoseNeutralCalibrator(HeadPoseNeutralConfig config) : config_(config)
{
    std::string error;
    valid_ = config_.validate(error);
}

void HeadPoseNeutralCalibrator::reset() noexcept
{
    calibrated_ = false;
    hasTimestamp_ = false;
    last_ = {};
    started_ = {};
    minYaw_ = maxYaw_ = minPitch_ = maxPitch_ = 0.0F;
    yawSum_ = pitchSum_ = 0.0;
    count_ = 0;
    neutralYaw_ = neutralPitch_ = 0.0F;
}

std::optional<HeadPoseAngles> HeadPoseNeutralCalibrator::update(dms::MonotonicTime timestamp,
                                                                const HeadPoseAngles &raw) noexcept
{
    if (!valid_ || !std::isfinite(raw.yawDegrees) || !std::isfinite(raw.pitchDegrees) ||
        (hasTimestamp_ && timestamp <= last_))
        return std::nullopt;
    hasTimestamp_ = true;
    last_ = timestamp;
    if (calibrated_)
    {
        HeadPoseAngles result = raw;
        result.yawDegrees -= neutralYaw_;
        result.pitchDegrees -= neutralPitch_;
        return result;
    }
    if (count_ == 0)
    {
        started_ = timestamp;
        minYaw_ = maxYaw_ = raw.yawDegrees;
        minPitch_ = maxPitch_ = raw.pitchDegrees;
    }
    else
    {
        minYaw_ = std::min(minYaw_, raw.yawDegrees);
        maxYaw_ = std::max(maxYaw_, raw.yawDegrees);
        minPitch_ = std::min(minPitch_, raw.pitchDegrees);
        maxPitch_ = std::max(maxPitch_, raw.pitchDegrees);
        if (maxYaw_ - minYaw_ > config_.maximumYawRange || maxPitch_ - minPitch_ > config_.maximumPitchRange)
        {
            started_ = timestamp;
            minYaw_ = maxYaw_ = raw.yawDegrees;
            minPitch_ = maxPitch_ = raw.pitchDegrees;
            yawSum_ = raw.yawDegrees;
            pitchSum_ = raw.pitchDegrees;
            count_ = 1;
            return std::nullopt;
        }
    }
    yawSum_ += raw.yawDegrees;
    pitchSum_ += raw.pitchDegrees;
    ++count_;
    if (timestamp - started_ < config_.confirmation)
        return std::nullopt;
    neutralYaw_ = static_cast<float>(yawSum_ / count_);
    neutralPitch_ = static_cast<float>(pitchSum_ / count_);
    calibrated_ = true;
    HeadPoseAngles result = raw;
    result.yawDegrees -= neutralYaw_;
    result.pitchDegrees -= neutralPitch_;
    return result;
}

bool estimateHeadPose(const SemanticFaceGeometry &geometry, cv::Size frameSize, HeadPoseAngles &result) noexcept
{
    result = {};
    if (frameSize.width <= 0 || frameSize.height <= 0)
        return false;
    const std::vector<cv::Point2d> imagePoints = {geometry.noseTip,          geometry.chin,
                                                  geometry.rightEyeOuter,    geometry.leftEyeOuter,
                                                  geometry.rightMouthCorner, geometry.leftMouthCorner};
    for (const auto &point : imagePoints)
        if (!finite(point))
            return false;

    const double focal = static_cast<double>(frameSize.width);
    const cv::Matx33d camera(focal, 0.0, frameSize.width * 0.5, 0.0, focal, frameSize.height * 0.5, 0.0, 0.0, 1.0);
    cv::Vec3d rotationVector, translationVector;
    try
    {
        if (!cv::solvePnP(kGenericFaceModel, imagePoints, camera, cv::noArray(), rotationVector, translationVector,
                          false, cv::SOLVEPNP_ITERATIVE))
            return false;
        cv::Matx33d rotation;
        cv::Rodrigues(rotationVector, rotation);
        cv::Matx33d upperTriangular, orthogonal;
        cv::Vec3d angles = cv::RQDecomp3x3(rotation, upperTriangular, orthogonal);
        std::vector<cv::Point2d> projected;
        cv::projectPoints(kGenericFaceModel, rotationVector, translationVector, camera, cv::noArray(), projected);
        double squaredError = 0.0;
        for (std::size_t index = 0; index < projected.size(); ++index)
        {
            const cv::Point2d delta = projected[index] - imagePoints[index];
            squaredError += delta.dot(delta);
        }
        result.pitchDegrees = static_cast<float>(angles[0]);
        result.yawDegrees = static_cast<float>(angles[1]);
        result.rollDegrees = static_cast<float>(angles[2]);
        result.reprojectionErrorPixels = static_cast<float>(std::sqrt(squaredError / projected.size()));
    }
    catch (const cv::Exception &)
    {
        result = {};
        return false;
    }
    return std::isfinite(result.yawDegrees) && std::isfinite(result.pitchDegrees) &&
           std::isfinite(result.rollDegrees) && std::isfinite(result.reprojectionErrorPixels);
}
