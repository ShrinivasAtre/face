#pragma once

#include "BackendFaceGeometryMapper.hpp"
#include "DmsObservation.hpp"

#include <opencv2/core.hpp>

#include <cstdint>
#include <optional>
#include <string>

struct HeadPoseAngles
{
    // DMS semantic convention: yaw is negative left / positive right;
    // pitch is negative up / positive down.
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
    float rollDegrees = 0.0F;
    float reprojectionErrorPixels = 0.0F;
};

// Estimates pose from provider-neutral semantic facial points. The generic 3D
// face model and camera approximation are replaceable calibration inputs later;
// no backend landmark topology is used here.
bool estimateHeadPose(const SemanticFaceGeometry &geometry, cv::Size frameSize, HeadPoseAngles &result) noexcept;

struct HeadPoseNeutralConfig
{
    dms::MonotonicTime confirmation = std::chrono::seconds(1);
    float maximumYawRange = 10.0F;
    float maximumPitchRange = 10.0F;
    bool validate(std::string &error) const noexcept;
};

class HeadPoseNeutralCalibrator
{
  public:
    explicit HeadPoseNeutralCalibrator(HeadPoseNeutralConfig config = {});
    std::optional<HeadPoseAngles> update(dms::MonotonicTime timestamp, const HeadPoseAngles &raw) noexcept;
    bool calibrated() const noexcept
    {
        return calibrated_;
    }
    void reset() noexcept;

  private:
    HeadPoseNeutralConfig config_;
    bool valid_ = false, calibrated_ = false;
    bool hasTimestamp_ = false;
    dms::MonotonicTime last_{}, started_{};
    float minYaw_ = 0, maxYaw_ = 0, minPitch_ = 0, maxPitch_ = 0;
    double yawSum_ = 0, pitchSum_ = 0;
    std::uint64_t count_ = 0;
    float neutralYaw_ = 0, neutralPitch_ = 0;
};
