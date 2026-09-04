#pragma once

#include "DmsEyeMetrics.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace dms
{
struct EyeStatisticsConfig
{
    MonotonicTime rollingWindow = std::chrono::minutes(5);
    MonotonicTime maximumSampleGap = std::chrono::milliseconds(250);
    float minimumKnownCoverage = 0.80F;
    bool cumulativeEnabled = true;
    bool rollingEnabled = true;

    bool validate(std::string &error) const noexcept;
};

struct EyeStatisticsResult
{
    bool updateAccepted = false;
    std::optional<float> cumulativeEyeOpen;
    float cumulativeKnownCoverage = 0.0F;
    std::uint64_t cumulativeBlinkCount = 0;
    std::optional<float> rollingEyeOpen;
    float rollingKnownCoverage = 0.0F;
    std::uint64_t rollingBlinkCount = 0;
};

class EyeStatistics
{
public:
    explicit EyeStatistics(EyeStatisticsConfig config);

    bool valid() const noexcept { return valid_; }
    const std::string &error() const noexcept { return error_; }
    EyeStatisticsResult update(MonotonicTime timestamp, EyeState state,
                               bool blinkEvent) noexcept;
    void reset() noexcept;

private:
    struct Interval
    {
        MonotonicTime start{};
        MonotonicTime end{};
        EyeState state = EyeState::Unknown;
    };

    EyeStatisticsResult result(MonotonicTime now, bool accepted) const noexcept;
    void trim(MonotonicTime now) noexcept;

    EyeStatisticsConfig config_;
    bool valid_ = false;
    std::string error_;
    bool hasTimestamp_ = false;
    MonotonicTime epoch_{}, lastTimestamp_{};
    EyeState lastState_ = EyeState::Unknown;
    MonotonicTime cumulativeKnown_{}, cumulativeOpen_{};
    std::uint64_t cumulativeBlinks_ = 0;
    std::deque<Interval> intervals_;
    std::deque<MonotonicTime> blinkEvents_;
};
}
