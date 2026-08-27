#pragma once

#include "DmsObservation.hpp"

#include <optional>

namespace dms
{
// Converts decoder presentation timestamps into a strictly monotonic recorded
// timeline. Duplicate, missing, or reset timestamps advance by the nominal
// frame period so temporal DMS logic never depends on wall-clock inference time.
class RecordedFrameClock
{
public:
    explicit RecordedFrameClock(double nominalFramesPerSecond = 30.0) noexcept;

    void reset(double nominalFramesPerSecond = 30.0) noexcept;
    MonotonicTime advance(
        std::optional<double> sourceTimestampMilliseconds) noexcept;
    MonotonicTime timestamp() const noexcept { return timestamp_; }

private:
    MonotonicTime framePeriod_{};
    MonotonicTime timestamp_{};
    std::optional<double> sourceOriginMilliseconds_;
    bool started_ = false;
};
}
