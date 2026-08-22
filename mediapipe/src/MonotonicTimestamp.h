#pragma once

#include <chrono>
#include <cstdint>

namespace face_mp_internal {

class MonotonicTimestamp
{
public:
    using Clock = std::chrono::steady_clock;

    MonotonicTimestamp() : start_(Clock::now()) {}

    int64_t next(Clock::time_point now)
    {
        const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_).count();
        last_ = elapsed > last_ ? elapsed : last_ + 1;
        return last_;
    }

private:
    Clock::time_point start_;
    int64_t last_ = -1;
};

}  // namespace face_mp_internal
