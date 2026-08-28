#include "DmsEyeMetrics.hpp"

#include <algorithm>
#include <cmath>

namespace dms
{
bool EyeOpenCalibrationConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (confirmation <= MonotonicTime::zero() || !std::isfinite(minimumCandidateEar) ||
        !std::isfinite(maximumEarRange) || !std::isfinite(closedToOpenRatio) || minimumCandidateEar <= 0.0F ||
        maximumEarRange <= 0.0F || closedToOpenRatio <= 0.0F || closedToOpenRatio >= 1.0F)
    { error = "invalid open-eye calibration configuration"; return false; }
    return true;
}
EyeOpenCalibrator::EyeOpenCalibrator(EyeOpenCalibrationConfig config) : config_(config)
{
    std::string error;
    valid_ = config_.validate(error);
}
void EyeOpenCalibrator::reset() noexcept
{
    hasTimestamp_ = false; last_ = started_ = {}; minimum_ = maximum_ = 0.0F; sum_ = 0.0; count_ = 0;
    calibration_.reset();
}
std::optional<EyeCalibration> EyeOpenCalibrator::update(MonotonicTime timestamp, ObservationUsability usability,
                                                        std::optional<float> rightEar,
                                                        std::optional<float> leftEar) noexcept
{
    if (!valid_ || (hasTimestamp_ && timestamp <= last_)) return std::nullopt;
    hasTimestamp_ = true; last_ = timestamp;
    if (calibration_) return calibration_;
    if (usability != ObservationUsability::Usable || !rightEar || !leftEar || !std::isfinite(*rightEar) ||
        !std::isfinite(*leftEar)) { count_ = 0; return std::nullopt; }
    const float ear = std::min(*rightEar, *leftEar);
    if (ear < config_.minimumCandidateEar) { count_ = 0; return std::nullopt; }
    if (!count_) { started_ = timestamp; minimum_ = maximum_ = ear; sum_ = 0.0; }
    minimum_ = std::min(minimum_, ear); maximum_ = std::max(maximum_, ear); sum_ += ear; ++count_;
    if (maximum_ - minimum_ > config_.maximumEarRange)
    { started_ = timestamp; minimum_ = maximum_ = ear; sum_ = ear; count_ = 1; return std::nullopt; }
    if (timestamp - started_ < config_.confirmation) return std::nullopt;
    const float open = static_cast<float>(sum_ / static_cast<double>(count_));
    calibration_ = EyeCalibration{open * config_.closedToOpenRatio, open};
    return calibration_;
}
bool EyeCalibration::validate(std::string& error) const noexcept
{
    error.clear();
    if (!std::isfinite(closedEar) || !std::isfinite(openEar) ||
        closedEar < 0.0F || openEar <= closedEar)
    {
        error = "eye calibration requires finite 0 <= closedEar < openEar";
        return false;
    }
    return true;
}

std::optional<float> EyeCalibration::normalize(float ear) const noexcept
{
    std::string ignored;
    if (!validate(ignored) || !std::isfinite(ear)) return std::nullopt;
    return std::clamp((ear - closedEar) / (openEar - closedEar), 0.0F, 1.0F);
}

bool EyeTemporalConfig::validate(std::string& error) const noexcept
{
    error.clear();
    if (!std::isfinite(closeOpenness) || !std::isfinite(reopenOpenness) ||
        closeOpenness < 0.0F || reopenOpenness > 1.0F ||
        closeOpenness >= reopenOpenness)
    {
        error = "eye openness thresholds must satisfy 0 <= close < reopen <= 1";
        return false;
    }
    if (minimumBlinkClosure <= MonotonicTime::zero() ||
        maximumBlinkClosure < minimumBlinkClosure ||
        maximumLongBlinkClosure <= maximumBlinkClosure ||
        maximumLongBlinkClosure > prolongedClosure ||
        reopenConfirmation <= MonotonicTime::zero() ||
        blinkRefractory < MonotonicTime::zero() ||
        prolongedClosure <= maximumBlinkClosure ||
        perclosWindow <= MonotonicTime::zero() ||
        maximumSampleGap <= MonotonicTime::zero() ||
        !std::isfinite(minimumPerclosCoverage) ||
        minimumPerclosCoverage < 0.0F || minimumPerclosCoverage > 1.0F)
    {
        error = "eye temporal durations or PERCLOS coverage are invalid";
        return false;
    }
    return true;
}

EyeTemporalMetrics::EyeTemporalMetrics(
    EyeCalibration calibration, EyeTemporalConfig config)
    : calibration_(calibration), config_(config)
{
    valid_ = calibration_.validate(error_) && config_.validate(error_);
}

bool EyeTemporalMetrics::setCalibration(EyeCalibration calibration) noexcept
{
    std::string error;
    if (!calibration.validate(error)) return false;
    calibration_ = calibration;
    reset();
    return true;
}

void EyeTemporalMetrics::reset() noexcept
{
    hasTimestamp_ = false;
    lastTimestamp_ = {};
    state_ = EyeState::Unknown;
    closureStarted_.reset();
    reopeningStarted_.reset();
    blinkCount_ = 0;
    longBlinkCount_ = 0;
    prolongedClosureCount_ = 0;
    refractoryUntil_.reset();
    prolongedClosureEmitted_ = false;
    intervals_.clear();
}

void EyeTemporalMetrics::appendInterval(MonotonicTime end) noexcept
{
    if (!hasTimestamp_ || end <= lastTimestamp_) return;
    const MonotonicTime gap = end - lastTimestamp_;
    intervals_.push_back({lastTimestamp_, end,
                          gap <= config_.maximumSampleGap ? state_
                                                         : EyeState::Unknown});
}

void EyeTemporalMetrics::trimIntervals(MonotonicTime now) noexcept
{
    const MonotonicTime windowStart = now - config_.perclosWindow;
    while (!intervals_.empty() && intervals_.front().end <= windowStart)
        intervals_.pop_front();
    if (!intervals_.empty() && intervals_.front().start < windowStart)
        intervals_.front().start = windowStart;
}

void EyeTemporalMetrics::updatePerclos(
    MonotonicTime now, EyeMetricResult& result) const noexcept
{
    MonotonicTime known{};
    MonotonicTime closed{};
    for (const Interval& interval : intervals_)
    {
        if (interval.state == EyeState::Unknown) continue;
        const MonotonicTime duration = interval.end - interval.start;
        known += duration;
        if (interval.state == EyeState::Closed) closed += duration;
    }
    const double window = static_cast<double>(config_.perclosWindow.count());
    result.perclosCoverage = window > 0.0
        ? static_cast<float>(static_cast<double>(known.count()) / window) : 0.0F;
    if (known > MonotonicTime::zero() &&
        result.perclosCoverage >= config_.minimumPerclosCoverage)
    {
        result.perclos = static_cast<float>(
            static_cast<double>(closed.count()) /
            static_cast<double>(known.count()));
    }
}

EyeMetricResult EyeTemporalMetrics::update(const EyeMetricInput& input) noexcept
{
    EyeMetricResult result;
    result.blinkCount = blinkCount_;
    result.longBlinkCount = longBlinkCount_;
    result.prolongedClosureCount = prolongedClosureCount_;
    if (!valid_) return result;
    if (hasTimestamp_ && input.timestamp <= lastTimestamp_) return result;

    appendInterval(input.timestamp);
    if (hasTimestamp_) trimIntervals(input.timestamp);

    std::optional<float> openness;
    if (input.usability == ObservationUsability::Usable &&
        input.rightEar && input.leftEar)
    {
        const auto right = calibration_.normalize(*input.rightEar);
        const auto left = calibration_.normalize(*input.leftEar);
        if (right && left) openness = std::min(*right, *left);
    }

    EyeState next = state_;
    if (!openness)
    {
        next = EyeState::Unknown;
    }
    else if (*openness <= config_.closeOpenness)
    {
        next = EyeState::Closed;
    }
    else if (*openness >= config_.reopenOpenness)
    {
        next = EyeState::Open;
    }

    if (next == EyeState::Unknown)
    {
        closureStarted_.reset();
        reopeningStarted_.reset();
        prolongedClosureEmitted_ = false;
    }
    else if (next == EyeState::Closed)
    {
        reopeningStarted_.reset();
        if (state_ != EyeState::Closed)
        {
            if (!refractoryUntil_ || input.timestamp >= *refractoryUntil_)
                closureStarted_ = input.timestamp;
            else
                closureStarted_.reset();
        }
    }
    else
    {
        if (state_ == EyeState::Closed && closureStarted_)
            reopeningStarted_ = input.timestamp;
        if (reopeningStarted_ &&
            input.timestamp - *reopeningStarted_ >= config_.reopenConfirmation)
        {
            const MonotonicTime closure = *reopeningStarted_ - *closureStarted_;
            const bool outsideRefractory = !refractoryUntil_ || input.timestamp >= *refractoryUntil_;
            if (outsideRefractory && closure >= config_.minimumBlinkClosure &&
                closure <= config_.maximumBlinkClosure)
            {
                ++blinkCount_;
                result.blinkEvent = true;
                refractoryUntil_ = input.timestamp + config_.blinkRefractory;
            }
            else if (outsideRefractory && closure > config_.maximumBlinkClosure &&
                     closure < config_.maximumLongBlinkClosure)
            {
                ++longBlinkCount_;
                result.longBlinkEvent = true;
                refractoryUntil_ = input.timestamp + config_.blinkRefractory;
            }
            closureStarted_.reset();
            reopeningStarted_.reset();
        }
    }

    state_ = next;
    hasTimestamp_ = true;
    lastTimestamp_ = input.timestamp;
    result.state = state_;
    result.openness = openness;
    result.blinkCount = blinkCount_;
    result.longBlinkCount = longBlinkCount_;
    result.prolongedClosureCount = prolongedClosureCount_;
    if (state_ == EyeState::Closed && closureStarted_)
    {
        result.closureDuration = input.timestamp - *closureStarted_;
        result.prolongedClosure =
            result.closureDuration >= config_.prolongedClosure;
        if (result.prolongedClosure && !prolongedClosureEmitted_)
        {
            prolongedClosureEmitted_ = true;
            ++prolongedClosureCount_;
            result.prolongedClosureCount = prolongedClosureCount_;
            result.prolongedClosureEvent = true;
        }
    }
    updatePerclos(input.timestamp, result);
    return result;
}
}
