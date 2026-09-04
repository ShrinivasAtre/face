#include "DmsEyeStatistics.hpp"

#include <algorithm>
#include <cmath>

namespace dms
{
bool EyeStatisticsConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (rollingWindow <= MonotonicTime::zero() ||
        maximumSampleGap <= MonotonicTime::zero() ||
        !std::isfinite(minimumKnownCoverage) ||
        minimumKnownCoverage < 0.0F || minimumKnownCoverage > 1.0F)
    {
        error = "invalid eye statistics window, sample gap, or coverage";
        return false;
    }
    return true;
}

EyeStatistics::EyeStatistics(EyeStatisticsConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}

void EyeStatistics::reset() noexcept
{
    hasTimestamp_ = false;
    epoch_ = lastTimestamp_ = {};
    lastState_ = EyeState::Unknown;
    cumulativeKnown_ = cumulativeOpen_ = {};
    cumulativeBlinks_ = 0;
    intervals_.clear();
    blinkEvents_.clear();
}

void EyeStatistics::trim(MonotonicTime now) noexcept
{
    const MonotonicTime start = now - config_.rollingWindow;
    while (!intervals_.empty() && intervals_.front().end <= start) intervals_.pop_front();
    if (!intervals_.empty() && intervals_.front().start < start) intervals_.front().start = start;
    while (!blinkEvents_.empty() && blinkEvents_.front() < start) blinkEvents_.pop_front();
}

EyeStatisticsResult EyeStatistics::result(MonotonicTime now, bool accepted) const noexcept
{
    EyeStatisticsResult output;
    output.updateAccepted = accepted;
    if (!hasTimestamp_) return output;
    const MonotonicTime elapsed = now - epoch_;
    if (elapsed > MonotonicTime::zero())
        output.cumulativeKnownCoverage = std::clamp(static_cast<float>(
            static_cast<double>(cumulativeKnown_.count()) / elapsed.count()), 0.0F, 1.0F);
    if (config_.cumulativeEnabled)
    {
        output.cumulativeBlinkCount = cumulativeBlinks_;
        if (cumulativeKnown_ > MonotonicTime::zero())
            output.cumulativeEyeOpen = static_cast<float>(
                static_cast<double>(cumulativeOpen_.count()) / cumulativeKnown_.count());
    }

    MonotonicTime rollingKnown{}, rollingOpen{};
    for (const Interval &interval : intervals_)
    {
        if (interval.state == EyeState::Unknown) continue;
        const MonotonicTime duration = interval.end - interval.start;
        rollingKnown += duration;
        if (interval.state == EyeState::Open) rollingOpen += duration;
    }
    const MonotonicTime availableWindow = std::min(config_.rollingWindow, elapsed);
    if (availableWindow > MonotonicTime::zero())
        output.rollingKnownCoverage = std::clamp(static_cast<float>(
            static_cast<double>(rollingKnown.count()) / availableWindow.count()), 0.0F, 1.0F);
    if (config_.rollingEnabled)
    {
        output.rollingBlinkCount = static_cast<std::uint64_t>(blinkEvents_.size());
        if (rollingKnown > MonotonicTime::zero() &&
            output.rollingKnownCoverage >= config_.minimumKnownCoverage)
            output.rollingEyeOpen = static_cast<float>(
                static_cast<double>(rollingOpen.count()) / rollingKnown.count());
    }
    return output;
}

EyeStatisticsResult EyeStatistics::update(MonotonicTime timestamp, EyeState state,
                                          bool blinkEvent) noexcept
{
    if (!valid_) return {};
    if (hasTimestamp_ && timestamp <= lastTimestamp_)
        return result(lastTimestamp_, false);
    if (!hasTimestamp_)
    {
        hasTimestamp_ = true;
        epoch_ = lastTimestamp_ = timestamp;
        lastState_ = state;
        if (blinkEvent) { ++cumulativeBlinks_; blinkEvents_.push_back(timestamp); }
        return result(timestamp, true);
    }

    const MonotonicTime duration = timestamp - lastTimestamp_;
    const EyeState intervalState = duration <= config_.maximumSampleGap
        ? lastState_ : EyeState::Unknown;
    intervals_.push_back({lastTimestamp_, timestamp, intervalState});
    if (intervalState != EyeState::Unknown)
    {
        cumulativeKnown_ += duration;
        if (intervalState == EyeState::Open) cumulativeOpen_ += duration;
    }
    if (blinkEvent) { ++cumulativeBlinks_; blinkEvents_.push_back(timestamp); }
    lastTimestamp_ = timestamp;
    lastState_ = state;
    trim(timestamp);
    return result(timestamp, true);
}
}
