#pragma once

#include "DmsObservation.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace dms
{
struct EyeCalibration
{
    float closedEar = 0.15F;
    float openEar = 0.30F;

    bool validate(std::string& error) const noexcept;
    std::optional<float> normalize(float ear) const noexcept;
};

struct EyeOpenCalibrationConfig
{
    MonotonicTime confirmation = std::chrono::seconds(2);
    float minimumCandidateEar = 0.18F;
    float maximumEarRange = 0.08F;
    float closedToOpenRatio = 0.50F;
    bool validate(std::string &error) const noexcept;
};

class EyeOpenCalibrator
{
  public:
    explicit EyeOpenCalibrator(EyeOpenCalibrationConfig config = {});
    std::optional<EyeCalibration> update(MonotonicTime timestamp, ObservationUsability usability,
                                         std::optional<float> rightEar, std::optional<float> leftEar) noexcept;
    bool calibrated() const noexcept { return calibration_.has_value(); }
    void reset() noexcept;
  private:
    EyeOpenCalibrationConfig config_;
    bool valid_ = false, hasTimestamp_ = false;
    MonotonicTime last_{}, started_{};
    float minimum_ = 0.0F, maximum_ = 0.0F;
    double sum_ = 0.0;
    std::uint64_t count_ = 0;
    std::optional<EyeCalibration> calibration_;
};

struct EyeTemporalConfig
{
    float closeOpenness = 0.25F;
    float reopenOpenness = 0.40F;
    MonotonicTime minimumBlinkClosure = std::chrono::milliseconds(80);
    MonotonicTime maximumBlinkClosure = std::chrono::milliseconds(700);
    MonotonicTime maximumLongBlinkClosure = std::chrono::milliseconds(1500);
    MonotonicTime reopenConfirmation = std::chrono::milliseconds(80);
    MonotonicTime blinkRefractory = std::chrono::milliseconds(150);
    MonotonicTime prolongedClosure = std::chrono::milliseconds(1500);
    MonotonicTime perclosWindow = std::chrono::seconds(60);
    MonotonicTime maximumSampleGap = std::chrono::milliseconds(250);
    float minimumPerclosCoverage = 0.80F;

    bool validate(std::string& error) const noexcept;
};

enum class EyeState
{
    Unknown,
    Open,
    Closed
};

struct EyeMetricInput
{
    MonotonicTime timestamp{};
    ObservationUsability usability = ObservationUsability::Missing;
    std::optional<float> rightEar;
    std::optional<float> leftEar;
};

struct EyeMetricResult
{
    EyeState state = EyeState::Unknown;
    std::optional<float> openness;
    std::optional<float> perclos;
    float perclosCoverage = 0.0F;
    std::uint64_t blinkCount = 0;
    bool blinkEvent = false;
    std::uint64_t longBlinkCount = 0;
    bool longBlinkEvent = false;
    bool prolongedClosure = false;
    std::uint64_t prolongedClosureCount = 0;
    bool prolongedClosureEvent = false;
    MonotonicTime closureDuration{};
};

class EyeTemporalMetrics
{
public:
    EyeTemporalMetrics(EyeCalibration calibration, EyeTemporalConfig config);

    bool valid() const noexcept { return valid_; }
    const std::string& error() const noexcept { return error_; }
    EyeMetricResult update(const EyeMetricInput& input) noexcept;
    bool setCalibration(EyeCalibration calibration) noexcept;
    const EyeCalibration& calibration() const noexcept { return calibration_; }
    void reset() noexcept;

private:
    struct Interval
    {
        MonotonicTime start{};
        MonotonicTime end{};
        EyeState state = EyeState::Unknown;
    };

    void appendInterval(MonotonicTime end) noexcept;
    void trimIntervals(MonotonicTime now) noexcept;
    void updatePerclos(MonotonicTime now, EyeMetricResult& result) const noexcept;

    EyeCalibration calibration_;
    EyeTemporalConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime lastTimestamp_{};
    EyeState state_ = EyeState::Unknown;
    std::optional<MonotonicTime> closureStarted_;
    std::optional<MonotonicTime> reopeningStarted_;
    std::uint64_t blinkCount_ = 0;
    std::uint64_t longBlinkCount_ = 0;
    std::uint64_t prolongedClosureCount_ = 0;
    std::optional<MonotonicTime> refractoryUntil_;
    bool prolongedClosureEmitted_ = false;
    std::deque<Interval> intervals_;
};
}
