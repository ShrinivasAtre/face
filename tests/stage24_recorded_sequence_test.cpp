#include "DmsEyeStatistics.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    using namespace std::chrono_literals;
    using namespace dms;
    struct Sample { MonotonicTime time; EyeState state; bool blink; };
    const std::vector<Sample> sequence = {
        {0ms, EyeState::Open, false}, {200ms, EyeState::Open, true},
        {400ms, EyeState::Open, false}, {600ms, EyeState::Unknown, false},
        {800ms, EyeState::Unknown, false}, {1000ms, EyeState::Open, false},
        {1201ms, EyeState::Open, true}};

    EyeStatisticsConfig config;
    config.rollingWindow = 1s;
    config.maximumSampleGap = 250ms;
    config.minimumKnownCoverage = 0.40F;
    EyeStatistics statistics(config);
    EyeStatisticsResult result;
    for (const auto &sample : sequence)
        result = statistics.update(sample.time, sample.state, sample.blink);

    if (!check(result.cumulativeBlinkCount == 2,
               "transient face loss does not reset cumulative blinks") ||
        !check(result.rollingBlinkCount == 1,
               "rolling window expires pre-loss blink") ||
        !check(result.cumulativeEyeOpen && std::abs(*result.cumulativeEyeOpen - 1.0F) < 0.001F,
               "unknown face-loss time excluded from openness") ||
        !check(result.cumulativeKnownCoverage > 0.60F && result.cumulativeKnownCoverage < 0.70F,
               "face-loss time reported through known coverage")) return 1;

    const auto duplicate = statistics.update(1201ms, EyeState::Closed, true);
    if (!check(!duplicate.updateAccepted && duplicate.cumulativeBlinkCount == 2,
               "duplicate recorded timestamp cannot add an event")) return 1;

    // Represents the confirmed-driver-change signal that the identity pipeline
    // will call after stable matching; ordinary face loss above did not call it.
    statistics.reset();
    const auto newDriverStart = statistics.update(1400ms, EyeState::Open, false);
    const auto newDriverBlink = statistics.update(1600ms, EyeState::Open, true);
    if (!check(!newDriverStart.cumulativeEyeOpen &&
               newDriverBlink.cumulativeBlinkCount == 1 &&
               newDriverBlink.rollingBlinkCount == 1,
               "explicit confirmed-driver reset starts a clean epoch")) return 1;

    std::cout << "Stage 24 recorded sequence tests PASSED\n";
    return 0;
}
