#include "DmsEyeMetrics.hpp"

#include <chrono>
#include <cmath>
#include <iostream>

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
    using namespace dms;

    EyeCalibration calibration{0.10F, 0.30F};
    EyeTemporalConfig config;
    config.minimumBlinkClosure = 80ms;
    config.maximumBlinkClosure = 500ms;
    config.maximumLongBlinkClosure = 1200ms;
    config.blinkRefractory = 150ms;
    config.reopenConfirmation = 80ms;
    config.prolongedClosure = 1500ms;
    config.perclosWindow = 1s;
    config.maximumSampleGap = 200ms;
    config.minimumPerclosCoverage = 0.80F;
    EyeTemporalMetrics metrics(calibration, config);
    if (!check(metrics.valid(), "valid configuration")) return 1;

    const auto sample = [&](auto time, float ear,
                            ObservationUsability usability =
                                ObservationUsability::Usable)
    {
        return metrics.update({time, usability, ear, ear});
    };

    if (!check(sample(0ms, 0.30F).state == EyeState::Open, "initial open") ||
        !check(sample(40ms, 0.20F).state == EyeState::Open,
               "hysteresis retains open") ||
        !check(sample(100ms, 0.11F).state == EyeState::Closed, "closed") ||
        !check(sample(220ms, 0.30F).blinkCount == 0, "reopen debounce") ||
        !check(sample(300ms, 0.30F).blinkEvent, "confirmed blink") ||
        !check(sample(310ms, 0.30F).blinkCount == 1, "single count")) return 1;

    sample(500ms, 0.11F);
    const auto longClosure = sample(2100ms, 0.11F);
    if (!check(longClosure.prolongedClosure, "prolonged closure") ||
        !check(longClosure.closureDuration == 1600ms, "closure duration")) return 1;

    const auto unknown = sample(2200ms, 0.30F, ObservationUsability::Occluded);
    if (!check(unknown.state == EyeState::Unknown, "occlusion is unknown") ||
        !check(unknown.blinkCount == 1, "occlusion cannot create blink")) return 1;

    metrics.reset();
    sample(0ms, 0.30F);
    sample(100ms, 0.11F);
    sample(900ms, 0.30F);
    const auto longBlink = sample(1000ms, 0.30F);
    if (!check(longBlink.longBlinkEvent && longBlink.longBlinkCount == 1,
               "long blink counted separately") ||
        !check(longBlink.blinkCount == 0, "long blink is not ordinary blink")) return 1;
    sample(1050ms, 0.11F);
    sample(1150ms, 0.30F);
    const auto refractory = sample(1250ms, 0.30F);
    if (!check(!refractory.blinkEvent && refractory.blinkCount == 0,
               "refractory suppresses split blink")) return 1;

    metrics.reset();
    sample(0ms, 0.30F);
    sample(100ms, 0.11F);
    const auto prolongedEvent = sample(1600ms, 0.11F);
    const auto prolongedHeld = sample(1700ms, 0.11F);
    if (!check(prolongedEvent.prolongedClosureEvent && prolongedEvent.prolongedClosureCount == 1,
               "prolonged closure emits once") ||
        !check(!prolongedHeld.prolongedClosureEvent && prolongedHeld.prolongedClosureCount == 1,
               "prolonged closure does not repeat")) return 1;

    metrics.reset();
    for (int index = 0; index <= 10; ++index)
    {
        const float ear = index < 5 ? 0.11F : 0.30F;
        sample(std::chrono::milliseconds(index * 100), ear);
    }
    const auto perclos = sample(1100ms, 0.30F);
    if (!check(perclos.perclos.has_value(), "PERCLOS coverage") ||
        !check(std::abs(*perclos.perclos - 0.4F) < 0.001F, "PERCLOS value"))
        return 1;

    const auto nonMonotonic = sample(1100ms, 0.11F);
    if (!check(nonMonotonic.state == EyeState::Unknown,
               "non-monotonic timestamp rejected without state output")) return 1;

    EyeTemporalConfig invalid = config;
    invalid.closeOpenness = invalid.reopenOpenness;
    EyeTemporalMetrics invalidMetrics(calibration, invalid);
    if (!check(!invalidMetrics.valid(), "invalid hysteresis rejected")) return 1;

    std::cout << "DMS eye metric tests PASSED\n";
    return 0;
}
