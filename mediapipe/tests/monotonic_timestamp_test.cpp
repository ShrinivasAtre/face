#include "../src/MonotonicTimestamp.h"

#include <chrono>
#include <iostream>

int main()
{
    using Timestamp = face_mp_internal::MonotonicTimestamp;
    Timestamp timestamp;
    const auto now = Timestamp::Clock::now();
    const auto first = timestamp.next(now);
    const auto second = timestamp.next(now);
    const auto earlier = timestamp.next(now - std::chrono::seconds(1));
    const auto later = timestamp.next(now + std::chrono::seconds(2));
    if (!(second > first && earlier > second && later > earlier))
    {
        std::cerr << "Timestamp sequence was not strictly monotonic.\n";
        return 1;
    }
    std::cout << "Monotonic timestamp test PASSED\n";
    return 0;
}
