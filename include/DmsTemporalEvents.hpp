#pragma once

#include "DmsEyeMetrics.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace dms
{
struct YawnConfig
{
    float openThreshold = 0.65F;
    float closeThreshold = 0.45F;
    MonotonicTime minimumDuration = std::chrono::milliseconds(800);
    MonotonicTime confirmation = std::chrono::milliseconds(120);
    bool validate(std::string &error) const noexcept;
};

struct YawnResult
{
    bool active = false;
    bool event = false;
    std::uint64_t count = 0;
};

class YawnFsm
{
  public:
    explicit YawnFsm(YawnConfig config);
    bool valid() const noexcept
    {
        return valid_;
    }
    const std::string &error() const noexcept
    {
        return error_;
    }
    YawnResult update(MonotonicTime timestamp, ObservationUsability usability,
                      std::optional<float> mouthOpenness) noexcept;
    void reset() noexcept;

  private:
    YawnConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime last_{};
    bool active_ = false;
    std::optional<MonotonicTime> candidate_;
    std::optional<MonotonicTime> opened_;
    std::uint64_t count_ = 0;
};

enum class HeadZone
{
    Unknown,
    Neutral,
    Left,
    Right,
    Up,
    Down
};
struct HeadPoseConfig
{
    float yawEnterDegrees = 25.0F, yawExitDegrees = 15.0F;
    float pitchEnterDegrees = 18.0F, pitchExitDegrees = 10.0F;
    MonotonicTime confirmation = std::chrono::milliseconds(150);
    bool validate(std::string &error) const noexcept;
};
struct HeadPoseResult
{
    HeadZone zone = HeadZone::Unknown;
    bool movementEvent = false;
    std::uint64_t leftCount = 0, rightCount = 0, upCount = 0, downCount = 0;
};
class HeadPoseFsm
{
  public:
    explicit HeadPoseFsm(HeadPoseConfig config);
    bool valid() const noexcept
    {
        return valid_;
    }
    const std::string &error() const noexcept
    {
        return error_;
    }
    HeadPoseResult update(MonotonicTime timestamp, ObservationUsability usability, std::optional<float> yawDegrees,
                          std::optional<float> pitchDegrees) noexcept;
    void reset() noexcept;

  private:
    HeadPoseConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime last_{};
    HeadZone zone_ = HeadZone::Unknown;
    HeadZone candidateZone_ = HeadZone::Unknown;
    std::optional<MonotonicTime> candidateSince_;
    bool neutralArmed_ = true;
    std::uint64_t left_ = 0, right_ = 0, up_ = 0, down_ = 0;
};

enum class GazeZone
{
    Unknown,
    Forward,
    Left,
    Right,
    Up,
    Down
};
struct DistractionConfig
{
    float horizontalEnter = 0.20F, horizontalExit = 0.12F;
    float verticalEnter = 0.25F, verticalExit = 0.15F;
    MonotonicTime gazeConfirmation = std::chrono::milliseconds(150);
    MonotonicTime distractionDuration = std::chrono::seconds(2);
    MonotonicTime recoveryDuration = std::chrono::milliseconds(500);
    bool validate(std::string &error) const noexcept;
};
struct DistractionResult
{
    GazeZone gaze = GazeZone::Unknown;
    bool distracted = false;
    bool event = false;
};
class DistractionFsm
{
  public:
    explicit DistractionFsm(DistractionConfig config);
    bool valid() const noexcept
    {
        return valid_;
    }
    const std::string &error() const noexcept
    {
        return error_;
    }
    DistractionResult update(MonotonicTime timestamp, ObservationUsability usability,
                             std::optional<float> horizontalGaze, std::optional<float> verticalGaze,
                             HeadZone headZone) noexcept;
    void reset() noexcept;

  private:
    DistractionConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime last_{};
    GazeZone gaze_ = GazeZone::Unknown;
    GazeZone candidate_ = GazeZone::Unknown;
    std::optional<MonotonicTime> candidateSince_;
    std::optional<MonotonicTime> awaySince_, recoverySince_;
    bool distracted_ = false;
};

enum class PresenceState
{
    Unknown,
    Present,
    Absent
};
struct PresenceConfig
{
    MonotonicTime presentConfirmation = std::chrono::milliseconds(300);
    MonotonicTime absentConfirmation = std::chrono::seconds(1);
    bool validate(std::string &error) const noexcept;
};

struct MonitoringAvailabilityConfig
{
    MonotonicTime recordAfter = std::chrono::milliseconds(500);
    MonotonicTime notifyAfter = std::chrono::seconds(2);
    bool validate(std::string &error) const noexcept;
};
struct MonitoringAvailabilityResult
{
    bool unavailable = false;
    bool recordEvent = false;
    bool notify = false;
    bool notifyEvent = false;
    std::uint64_t episodeCount = 0;
    MonotonicTime unavailableDuration{};
};
class MonitoringAvailabilityFsm
{
  public:
    explicit MonitoringAvailabilityFsm(MonitoringAvailabilityConfig config);
    bool valid() const noexcept { return valid_; }
    MonitoringAvailabilityResult update(MonotonicTime timestamp, ObservationUsability usability) noexcept;
    void reset() noexcept;
  private:
    MonitoringAvailabilityConfig config_;
    bool valid_ = false, hasTimestamp_ = false, recorded_ = false, notified_ = false;
    MonotonicTime last_{}, unavailableSince_{};
    std::uint64_t count_ = 0;
};
class DriverPresenceFsm
{
  public:
    explicit DriverPresenceFsm(PresenceConfig config);
    PresenceState update(MonotonicTime timestamp, ObservationUsability usability, bool driverDetected) noexcept;
    void reset() noexcept;

  private:
    PresenceConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime last_{};
    PresenceState state_ = PresenceState::Unknown;
    PresenceState candidate_ = PresenceState::Unknown;
    std::optional<MonotonicTime> since_;
};

enum class DrowsinessState
{
    Unknown,
    Normal,
    Warning,
    Drowsy
};
struct DrowsinessConfig
{
    float perclosWarning = 0.20F, perclosDrowsy = 0.35F;
    MonotonicTime yawnEvidenceWindow = std::chrono::seconds(60);
    std::uint32_t yawnsForWarning = 2;
    MonotonicTime minimumAlertHold = std::chrono::seconds(5);
    MonotonicTime recoveryDuration = std::chrono::seconds(10);
    bool validate(std::string &error) const noexcept;
};
struct DrowsinessInput
{
    MonotonicTime timestamp{};
    ObservationUsability usability = ObservationUsability::Missing;
    PresenceState presence = PresenceState::Unknown;
    std::optional<float> perclos;
    bool prolongedClosure = false;
    bool longBlinkEvent = false;
    bool yawnEvent = false;
};
struct DrowsinessResult
{
    DrowsinessState state = DrowsinessState::Unknown;
    std::uint32_t recentYawns = 0;
};
class DrowsinessFsm
{
  public:
    explicit DrowsinessFsm(DrowsinessConfig config);
    DrowsinessResult update(const DrowsinessInput &input) noexcept;
    void reset() noexcept;

  private:
    DrowsinessConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime last_{};
    DrowsinessState state_ = DrowsinessState::Unknown;
    std::optional<MonotonicTime> recoverySince_;
    std::optional<MonotonicTime> alertSince_;
    std::deque<MonotonicTime> yawns_;
};
} // namespace dms
