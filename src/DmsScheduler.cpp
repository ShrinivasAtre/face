#include "DmsScheduler.hpp"

#include <cmath>

namespace dms
{
namespace
{
constexpr std::size_t indexOf(TaskKind task) noexcept
{
    return static_cast<std::size_t>(task);
}
}

SchedulerConfig SchedulerConfig::defaults() noexcept
{
    using namespace std::chrono_literals;
    SchedulerConfig result;
    result.tasks[indexOf(TaskKind::Eye)] = {33ms, {0.5F, 0.5F, 100ms}, true};
    result.tasks[indexOf(TaskKind::Geometry)] = {100ms, {0.5F, 0.5F, 250ms}, true};
    result.tasks[indexOf(TaskKind::Recognition)] = {500ms, {0.7F, 0.7F, 1500ms}, false};
    result.tasks[indexOf(TaskKind::Object)] = {250ms, {0.5F, 0.5F, 750ms}, false};
    return result;
}

bool SchedulerConfig::validate(std::string& error) const
{
    for (const TaskSchedule& task : tasks)
    {
        if (task.cadence <= MonotonicTime::zero())
        {
            error = "task cadence must be positive";
            return false;
        }
        if (task.observationPolicy.maximumAge < MonotonicTime::zero())
        {
            error = "maximum observation age cannot be negative";
            return false;
        }
        const float confidence = task.observationPolicy.minimumConfidence;
        const float visibility = task.observationPolicy.minimumVisibility;
        if (!std::isfinite(confidence) || confidence < 0.0F || confidence > 1.0F ||
            !std::isfinite(visibility) || visibility < 0.0F || visibility > 1.0F)
        {
            error = "minimum confidence and visibility must be finite and in [0, 1]";
            return false;
        }
    }
    error.clear();
    return true;
}

const TaskSchedule& SchedulerConfig::forTask(TaskKind task) const noexcept
{
    return tasks[indexOf(task)];
}

CadenceScheduler::CadenceScheduler(SchedulerConfig config)
    : config_(std::move(config))
{
}

ScheduleDecision CadenceScheduler::evaluate(
    TaskKind task,
    MonotonicTime now,
    bool observationUncertain) const noexcept
{
    const auto& lastRun = lastRuns_[indexOf(task)];
    if (!lastRun) return {true, ScheduleReason::FirstRun};
    if (now < *lastRun) return {false, ScheduleReason::NonMonotonicTime};
    const TaskSchedule& schedule = config_.forTask(task);
    if (observationUncertain && schedule.runOnUncertainty)
    {
        return {true, ScheduleReason::UncertainObservation};
    }
    if (now - *lastRun >= schedule.cadence)
    {
        return {true, ScheduleReason::CadenceElapsed};
    }
    return {false, ScheduleReason::NotDue};
}

bool CadenceScheduler::markRun(TaskKind task, MonotonicTime now) noexcept
{
    auto& lastRun = lastRuns_[indexOf(task)];
    if (lastRun && now < *lastRun) return false;
    lastRun = now;
    return true;
}

void CadenceScheduler::reset() noexcept
{
    for (auto& lastRun : lastRuns_) lastRun.reset();
}
}
