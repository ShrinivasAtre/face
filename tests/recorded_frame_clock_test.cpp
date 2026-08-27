#include "RecordedFrameClock.hpp"

#include <chrono>
#include <iostream>
#include <limits>

namespace
{
bool check(bool condition, const char* message)
{
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    using namespace std::chrono_literals;
    using dms::RecordedFrameClock;

    RecordedFrameClock clock(25.0);
    if (!check(clock.advance(1000.0) == 0ns, "first frame starts at zero") ||
        !check(clock.advance(1040.0) == 40ms, "source cadence retained") ||
        !check(clock.advance(1040.0) == 80ms, "duplicate advances nominally") ||
        !check(clock.advance(1030.0) == 120ms, "backward timestamp repaired") ||
        !check(clock.advance(std::nullopt) == 160ms, "missing timestamp repaired") ||
        !check(clock.advance(1250.0) == 250ms, "later source time catches up"))
        return 1;

    clock.reset(50.0);
    if (!check(clock.advance(std::numeric_limits<double>::quiet_NaN()) == 0ns,
               "invalid first timestamp") ||
        !check(clock.advance(std::nullopt) == 20ms, "configured cadence"))
        return 1;

    clock.reset(0.0);
    if (!check(clock.advance(0.0) == 0ns, "fallback start") ||
        !check(clock.advance(0.0) > 33ms && clock.timestamp() < 34ms,
               "invalid frame rate uses 30 FPS fallback"))
        return 1;

    std::cout << "Recorded frame clock tests PASSED\n";
    return 0;
}
