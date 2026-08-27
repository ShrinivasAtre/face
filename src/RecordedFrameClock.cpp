#include "RecordedFrameClock.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace dms
{
namespace
{
constexpr double fallbackFramesPerSecond = 30.0;

MonotonicTime framePeriod(double framesPerSecond) noexcept
{
    if (!std::isfinite(framesPerSecond) || framesPerSecond < 1.0 ||
        framesPerSecond > 1000.0)
    {
        framesPerSecond = fallbackFramesPerSecond;
    }
    return std::chrono::duration_cast<MonotonicTime>(
        std::chrono::duration<double>(1.0 / framesPerSecond));
}
}

RecordedFrameClock::RecordedFrameClock(double nominalFramesPerSecond) noexcept
{
    reset(nominalFramesPerSecond);
}

void RecordedFrameClock::reset(double nominalFramesPerSecond) noexcept
{
    framePeriod_ = framePeriod(nominalFramesPerSecond);
    timestamp_ = MonotonicTime::zero();
    sourceOriginMilliseconds_.reset();
    started_ = false;
}

MonotonicTime RecordedFrameClock::advance(
    std::optional<double> sourceTimestampMilliseconds) noexcept
{
    const bool sourceValid = sourceTimestampMilliseconds &&
        std::isfinite(*sourceTimestampMilliseconds) &&
        *sourceTimestampMilliseconds >= 0.0;
    if (!started_)
    {
        started_ = true;
        if (sourceValid) sourceOriginMilliseconds_ = *sourceTimestampMilliseconds;
        return timestamp_;
    }

    const MonotonicTime fallback = timestamp_ + framePeriod_;
    timestamp_ = fallback;
    if (sourceValid && sourceOriginMilliseconds_)
    {
        const double elapsedMilliseconds =
            *sourceTimestampMilliseconds - *sourceOriginMilliseconds_;
        if (std::isfinite(elapsedMilliseconds) && elapsedMilliseconds >= 0.0)
        {
            const auto sourceTime = std::chrono::duration_cast<MonotonicTime>(
                std::chrono::duration<double, std::milli>(elapsedMilliseconds));
            timestamp_ = std::max(timestamp_, sourceTime);
        }
    }
    return timestamp_;
}
}
