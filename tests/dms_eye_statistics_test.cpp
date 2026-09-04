#include "DmsEyeStatistics.hpp"

#include <chrono>
#include <cmath>
#include <iostream>

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
    EyeStatisticsConfig config;
    config.rollingWindow = 1s;
    config.maximumSampleGap = 250ms;
    config.minimumKnownCoverage = 0.75F;
    EyeStatistics statistics(config);
    if (!check(statistics.valid(), "valid configuration")) return 1;

    statistics.update(0ms, EyeState::Open, false);
    statistics.update(200ms, EyeState::Open, true);
    statistics.update(400ms, EyeState::Closed, false);
    statistics.update(600ms, EyeState::Closed, false);
    const auto balanced = statistics.update(800ms, EyeState::Open, true);
    if (!check(balanced.cumulativeEyeOpen && std::abs(*balanced.cumulativeEyeOpen - 0.5F) < 0.001F,
               "cumulative duration-weighted openness") ||
        !check(balanced.rollingEyeOpen && std::abs(*balanced.rollingEyeOpen - 0.5F) < 0.001F,
               "rolling duration-weighted openness") ||
        !check(balanced.cumulativeBlinkCount == 2 && balanced.rollingBlinkCount == 2,
               "cumulative and rolling blink counts")) return 1;

    const auto gap = statistics.update(1400ms, EyeState::Open, false);
    if (!check(gap.cumulativeKnownCoverage < 0.60F, "long gap excluded from known coverage") ||
        !check(!gap.rollingEyeOpen, "rolling openness gated by coverage")) return 1;
    const auto trimmed = statistics.update(1600ms, EyeState::Open, false);
    if (!check(trimmed.rollingBlinkCount == 1, "old blink timestamp trimmed")) return 1;

    const auto rejected = statistics.update(1600ms, EyeState::Closed, true);
    if (!check(!rejected.updateAccepted && rejected.cumulativeBlinkCount == 2,
               "non-monotonic update rejected without mutation")) return 1;

    statistics.reset();
    const auto reset = statistics.update(2s, EyeState::Open, false);
    if (!check(reset.cumulativeBlinkCount == 0 && !reset.cumulativeEyeOpen,
               "explicit reset starts a new epoch")) return 1;

    EyeStatisticsConfig disabledConfig = config;
    disabledConfig.cumulativeEnabled = false;
    disabledConfig.rollingEnabled = false;
    EyeStatistics disabled(disabledConfig);
    disabled.update(0ms, EyeState::Open, true);
    const auto hidden = disabled.update(100ms, EyeState::Open, true);
    if (!check(!hidden.cumulativeEyeOpen && !hidden.rollingEyeOpen &&
               hidden.cumulativeBlinkCount == 0 && hidden.rollingBlinkCount == 0,
               "disabled outputs suppressed")) return 1;

    EyeStatisticsConfig invalid = config;
    invalid.rollingWindow = 0ms;
    if (!check(!EyeStatistics(invalid).valid(), "invalid window rejected")) return 1;

    std::cout << "DMS eye statistics tests PASSED\n";
    return 0;
}
