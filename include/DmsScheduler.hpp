#pragma once

#include "DmsObservation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace dms
{
enum class TaskKind : std::size_t
{
    Eye = 0,
    Geometry,
    Recognition,
    Object,
    Count
};

struct TaskSchedule
{
    MonotonicTime cadence{};
    ObservationPolicy observationPolicy;
    bool runOnUncertainty = false;
};

struct SchedulerConfig
{
    std::array<TaskSchedule, static_cast<std::size_t>(TaskKind::Count)> tasks{};

    static SchedulerConfig defaults() noexcept;
    bool validate(std::string& error) const;
    const TaskSchedule& forTask(TaskKind task) const noexcept;
};

enum class ScheduleReason
{
    NotDue,
    FirstRun,
    CadenceElapsed,
    UncertainObservation,
    NonMonotonicTime
};

struct ScheduleDecision
{
    bool run = false;
    ScheduleReason reason = ScheduleReason::NotDue;
};

class CadenceScheduler
{
public:
    explicit CadenceScheduler(SchedulerConfig config);

    ScheduleDecision evaluate(
        TaskKind task,
        MonotonicTime now,
        bool observationUncertain) const noexcept;
    bool markRun(TaskKind task, MonotonicTime now) noexcept;
    void reset() noexcept;

private:
    SchedulerConfig config_;
    std::array<std::optional<MonotonicTime>,
               static_cast<std::size_t>(TaskKind::Count)> lastRuns_{};
};

template <typename Payload>
class LatestFrameSlot
{
public:
    struct Frame
    {
        FrameStamp stamp;
        Payload payload;
    };

    struct Counters
    {
        std::uint64_t published = 0;
        std::uint64_t consumed = 0;
        std::uint64_t superseded = 0;
    };

    void publish(Frame frame)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++counters_.published;
        if (latest_) ++counters_.superseded;
        latest_ = std::move(frame);
    }

    std::optional<Frame> takeLatest()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!latest_) return std::nullopt;
        std::optional<Frame> result(std::move(latest_));
        latest_.reset();
        ++counters_.consumed;
        return result;
    }

    std::size_t depth() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return latest_ ? 1U : 0U;
    }

    Counters counters() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return counters_;
    }

private:
    mutable std::mutex mutex_;
    std::optional<Frame> latest_;
    Counters counters_;
};
}
