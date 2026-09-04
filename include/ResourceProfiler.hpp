#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct ResourceSample
{
    double elapsedMilliseconds = 0.0;
    std::string phase;
    double processCpuPercentOfTotalCapacity = 0.0;
    std::uint64_t residentMemoryBytes = 0;
    std::uint64_t privateMemoryBytes = 0;
    std::size_t threadCount = 0;
    std::vector<double> systemCpuPercentPerCore;
};

struct ResourceSummary
{
    std::size_t samples = 0;
    double processCpuMean = 0.0;
    double processCpuPeak = 0.0;
    double systemCpuMean = 0.0;
    double systemCpuPeakCore = 0.0;
    std::uint64_t residentMemoryMinimumBytes = 0;
    std::uint64_t residentMemoryMaximumBytes = 0;
    std::uint64_t privateMemoryMaximumBytes = 0;
    std::size_t threadCountMaximum = 0;
};

class ResourceProfiler
{
  public:
    explicit ResourceProfiler(std::chrono::milliseconds interval);
    ~ResourceProfiler();

    ResourceProfiler(const ResourceProfiler&) = delete;
    ResourceProfiler& operator=(const ResourceProfiler&) = delete;

    void start();
    void setPhase(std::string phase);
    void stop();
    const std::vector<ResourceSample>& samples() const noexcept { return samples_; }
    ResourceSummary summarize(const std::string& phase = {}) const;
    bool writeCsv(const std::filesystem::path& path, std::string& error) const;

  private:
    void run();

    std::chrono::milliseconds interval_;
    std::chrono::steady_clock::time_point startedAt_;
    mutable std::mutex mutex_;
    std::string phase_ = "startup";
    bool stopRequested_ = false;
    std::thread worker_;
    std::vector<ResourceSample> samples_;
};
