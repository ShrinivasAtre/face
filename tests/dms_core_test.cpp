#include "DmsObservation.hpp"
#include "DmsScheduler.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
} // namespace

int main()
{
    using namespace std::chrono_literals;
    using namespace dms;

    SchedulerConfig config = SchedulerConfig::defaults();
    std::string error;
    if (!check(config.validate(error), "default configuration must validate"))
        return 1;

    CadenceScheduler scheduler(config);
    const MonotonicTime start = 10s;
    if (!check(scheduler.evaluate(TaskKind::Eye, start, false).reason == ScheduleReason::FirstRun,
               "eye must run initially") ||
        !check(scheduler.markRun(TaskKind::Eye, start), "initial mark must pass") ||
        !check(!scheduler.evaluate(TaskKind::Eye, start + 32ms, false).run, "eye must wait for its cadence") ||
        !check(scheduler.evaluate(TaskKind::Eye, start + 33ms, false).run, "eye must run at its cadence") ||
        !check(scheduler.evaluate(TaskKind::Eye, start + 1ms, true).reason == ScheduleReason::UncertainObservation,
               "uncertainty must trigger configured eye work") ||
        !check(scheduler.markRun(TaskKind::Recognition, start), "initial recognition mark must pass") ||
        !check(!scheduler.evaluate(TaskKind::Recognition, start + 1ms, true).run,
               "uncertainty must not bypass recognition cadence"))
    {
        return 1;
    }

    if (!check(scheduler.markRun(TaskKind::Geometry, start), "geometry mark") ||
        !check(scheduler.markRun(TaskKind::Object, start), "object mark"))
        return 1;

    const std::vector<MonotonicTime> recordedFrames = {start + 20ms, start + 40ms, start + 100ms, start + 260ms,
                                                       start + 500ms};
    std::array<int, static_cast<std::size_t>(TaskKind::Count)> runs{};
    for (MonotonicTime timestamp : recordedFrames)
    {
        for (TaskKind task : {TaskKind::Eye, TaskKind::Geometry, TaskKind::Recognition, TaskKind::Object})
        {
            if (scheduler.evaluate(task, timestamp, false).run)
            {
                ++runs[static_cast<std::size_t>(task)];
                if (!check(scheduler.markRun(task, timestamp), "recorded mark"))
                    return 1;
            }
        }
    }
    if (!check(runs[static_cast<std::size_t>(TaskKind::Eye)] == 4, "eye recorded cadence") ||
        !check(runs[static_cast<std::size_t>(TaskKind::Geometry)] == 3, "geometry recorded cadence") ||
        !check(runs[static_cast<std::size_t>(TaskKind::Recognition)] == 1, "recognition recorded cadence") ||
        !check(runs[static_cast<std::size_t>(TaskKind::Object)] == 1, "object recorded cadence"))
        return 1;

    ObservationHeader header{{42, start}, start + 1ms, 1ms, SourceValidity::Valid, 0.9F, 0.8F};
    const ObservationPolicy policy{0.7F, 0.6F, 100ms};
    if (!check(classifyObservation(header, start + 50ms, policy) == ObservationUsability::Usable,
               "valid observation") ||
        !check(classifyObservation(header, start + 101ms, policy) == ObservationUsability::Stale,
               "stale observation") ||
        !check(classifyObservation(header, start - 1ms, policy) == ObservationUsability::FutureTimestamp,
               "future observation"))
        return 1;
    header.confidence = 0.6F;
    if (!check(classifyObservation(header, start + 50ms, policy) == ObservationUsability::LowConfidence,
               "low-confidence observation"))
        return 1;
    header.confidence = 0.9F;
    header.visibility = 0.5F;
    if (!check(classifyObservation(header, start + 50ms, policy) == ObservationUsability::LowConfidence,
               "low-visibility observation"))
        return 1;
    header.visibility = 0.8F;
    header.validity = SourceValidity::Occluded;
    if (!check(classifyObservation(header, start + 50ms, policy) == ObservationUsability::Occluded,
               "occluded observation"))
        return 1;
    header.validity = SourceValidity::Missing;
    if (!check(classifyObservation(header, start + 50ms, policy) == ObservationUsability::Missing,
               "missing observation"))
        return 1;

    ObservationQualityGateConfig gateConfig;
    gateConfig.policy = policy;
    gateConfig.reacquisitionConfirmation = 100ms;
    ObservationQualityGate qualityGate(gateConfig);
    if (!check(qualityGate.valid(), "valid quality gate") ||
        !check(qualityGate.update({{1, start}, start, {}, SourceValidity::Valid, 0.9F, 0.8F}, start) ==
                   ObservationUsability::Recovering,
               "startup confirmation") ||
        !check(qualityGate.update({{2, start + 50ms}, start + 50ms, {}, SourceValidity::Valid, 0.9F, 0.8F},
                                  start + 50ms) == ObservationUsability::Recovering,
               "recovery remains gated") ||
        !check(qualityGate.update({{3, start + 100ms}, start + 100ms, {}, SourceValidity::Valid, 0.9F, 0.8F},
                                  start + 100ms) == ObservationUsability::Usable,
               "recovery confirmation") ||
        !check(qualityGate.update({{4, start + 110ms}, start + 110ms, {}, SourceValidity::Occluded, 1.0F, 1.0F},
                                  start + 110ms) == ObservationUsability::Occluded,
               "occlusion applies immediately") ||
        !check(qualityGate.update({{5, start + 120ms}, start + 120ms, {}, SourceValidity::Valid, 0.9F, 0.8F},
                                  start + 120ms) == ObservationUsability::Recovering,
               "reacquisition is gated") ||
        !check(qualityGate.update({{6, start + 220ms}, start + 220ms, {}, SourceValidity::Valid, 0.9F, 0.8F},
                                  start + 220ms) == ObservationUsability::Usable,
               "reacquisition confirms") ||
        !check(qualityGate.update({{7, start + 220ms}, start + 220ms, {}, SourceValidity::Valid, 0.9F, 0.8F},
                                  start + 220ms) == ObservationUsability::FutureTimestamp,
               "non-monotonic quality update rejected"))
        return 1;

    LatestFrameSlot<int> slot;
    slot.publish({{1, start}, 10});
    slot.publish({{2, start + 1ms}, 20});
    slot.publish({{3, start + 2ms}, 30});
    const auto latest = slot.takeLatest();
    const auto counters = slot.counters();
    if (!check(latest && latest->stamp.frameId == 3 && latest->payload == 30,
               "latest frame must supersede older work") ||
        !check(slot.depth() == 0, "slot must be empty after take") ||
        !check(counters.published == 3 && counters.consumed == 1 && counters.superseded == 2, "bounded slot counters"))
        return 1;

    config.tasks[0].cadence = MonotonicTime::zero();
    if (!check(!config.validate(error), "zero cadence must fail"))
        return 1;
    config = SchedulerConfig::defaults();
    config.tasks[0].observationPolicy.minimumVisibility = std::numeric_limits<float>::quiet_NaN();
    if (!check(!config.validate(error), "non-finite visibility must fail"))
        return 1;

    std::cout << "DMS core tests PASSED\n";
    return 0;
}
