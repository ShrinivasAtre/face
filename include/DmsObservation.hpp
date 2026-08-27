#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dms
{
using MonotonicTime = std::chrono::nanoseconds;

struct FrameStamp
{
    std::uint64_t frameId = 0;
    MonotonicTime capturedAt{};
};

enum class SourceValidity
{
    Missing,
    Valid,
    Occluded
};

struct ObservationHeader
{
    FrameStamp source;
    MonotonicTime producedAt{};
    MonotonicTime sourceAgeAtProduction{};
    SourceValidity validity = SourceValidity::Missing;
    float confidence = 0.0F;
    float visibility = 0.0F;
};

enum class ObservationUsability
{
    Usable,
    Recovering,
    Missing,
    LowConfidence,
    Occluded,
    Stale,
    FutureTimestamp
};

struct ObservationPolicy
{
    float minimumConfidence = 0.0F;
    float minimumVisibility = 0.0F;
    MonotonicTime maximumAge{};
};

ObservationUsability classifyObservation(const ObservationHeader &header, MonotonicTime now,
                                         const ObservationPolicy &policy) noexcept;

struct ObservationQualityGateConfig
{
    ObservationPolicy policy;
    MonotonicTime reacquisitionConfirmation = std::chrono::milliseconds(100);

    bool validate(std::string &error) const noexcept;
};

// Applies observation quality immediately on loss and requires a sustained
// usable interval after startup or reacquisition. This prevents an occlusion
// boundary from being interpreted as a valid temporal event.
class ObservationQualityGate
{
  public:
    explicit ObservationQualityGate(ObservationQualityGateConfig config);

    bool valid() const noexcept
    {
        return valid_;
    }
    const std::string &error() const noexcept
    {
        return error_;
    }
    ObservationUsability update(const ObservationHeader &header, MonotonicTime now) noexcept;
    void reset() noexcept;

  private:
    ObservationQualityGateConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime lastTimestamp_{};
    std::optional<MonotonicTime> usableSince_;
    bool usableConfirmed_ = false;
};

struct Point2f
{
    float x = 0.0F;
    float y = 0.0F;
};

struct EyeGeometry
{
    std::vector<Point2f> rightContour;
    std::vector<Point2f> leftContour;
    std::optional<float> rightOpenness;
    std::optional<float> leftOpenness;
};

struct FaceGeometry
{
    std::vector<Point2f> stablePosePoints;
    std::vector<Point2f> mouthContour;
};

enum class DriverPresence
{
    Unknown,
    Absent,
    Present
};

struct PresenceValue
{
    DriverPresence state = DriverPresence::Unknown;
};

struct RecognitionValue
{
    std::string driverId;
    bool known = false;
};

struct ObjectValue
{
    std::string className;
    float associationConfidence = 0.0F;
};

template <typename Value> struct Observation
{
    ObservationHeader header;
    Value value;
};
} // namespace dms
