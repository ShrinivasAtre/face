#include "ResourceProfiler.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <numeric>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#else
#include <dirent.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace
{
struct CpuTimes { std::uint64_t idle = 0, total = 0; };

#ifdef _WIN32
std::uint64_t fileTimeValue(const FILETIME& value)
{
    ULARGE_INTEGER converted{};
    converted.LowPart = value.dwLowDateTime;
    converted.HighPart = value.dwHighDateTime;
    return converted.QuadPart;
}

std::uint64_t processCpuTime()
{
    FILETIME created{}, exited{}, kernel{}, user{};
    return GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)
        ? fileTimeValue(kernel) + fileTimeValue(user) : 0;
}

std::vector<CpuTimes> cpuTimes()
{
    struct ProcessorTimes
    {
        LARGE_INTEGER idle, kernel, user, dpc, interrupt;
        ULONG interruptCount;
    };
    using QuerySystemInformation = LONG (WINAPI*)(ULONG, PVOID, ULONG, PULONG);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto query = ntdll ? reinterpret_cast<QuerySystemInformation>(
        GetProcAddress(ntdll, "NtQuerySystemInformation")) : nullptr;
    const unsigned int count = std::max(1U, std::thread::hardware_concurrency());
    std::vector<ProcessorTimes> raw(count);
    if (!query || query(8, raw.data(), static_cast<ULONG>(raw.size() * sizeof(ProcessorTimes)), nullptr) < 0)
        return {};
    std::vector<CpuTimes> result;
    result.reserve(raw.size());
    for (const auto& item : raw)
        result.push_back({static_cast<std::uint64_t>(item.idle.QuadPart),
                          static_cast<std::uint64_t>(item.kernel.QuadPart + item.user.QuadPart)});
    return result;
}

void memory(std::uint64_t& resident, std::uint64_t& privateBytes)
{
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)))
    {
        resident = counters.WorkingSetSize;
        privateBytes = counters.PrivateUsage;
    }
}

std::size_t threadCount()
{
    std::size_t count = 0;
    const DWORD processId = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry))
        do { if (entry.th32OwnerProcessID == processId) ++count; } while (Thread32Next(snapshot, &entry));
    CloseHandle(snapshot);
    return count;
}
#else
std::uint64_t processCpuTime()
{
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
    const auto micros = [](const timeval& value) {
        return static_cast<std::uint64_t>(value.tv_sec) * 1000000ULL + value.tv_usec;
    };
    return micros(usage.ru_utime) + micros(usage.ru_stime);
}

std::vector<CpuTimes> cpuTimes()
{
    std::ifstream file("/proc/stat");
    std::vector<CpuTimes> result;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.rfind("cpu", 0) != 0 || line.size() < 4 || line[3] < '0' || line[3] > '9') continue;
        std::istringstream fields(line);
        std::string name;
        std::uint64_t user = 0, nice = 0, system = 0, idle = 0, ioWait = 0,
                      irq = 0, softIrq = 0, steal = 0;
        fields >> name >> user >> nice >> system >> idle >> ioWait >> irq >> softIrq >> steal;
        result.push_back({idle + ioWait, user + nice + system + idle + ioWait + irq + softIrq + steal});
    }
    return result;
}

void memory(std::uint64_t& resident, std::uint64_t& privateBytes)
{
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line))
    {
        std::istringstream fields(line);
        std::string key, unit;
        std::uint64_t value = 0;
        if (!(fields >> key >> value >> unit)) continue;
        if (key == "VmRSS:") resident = value * 1024ULL;
        else if (key == "RssAnon:") privateBytes = value * 1024ULL;
    }
}

std::size_t threadCount()
{
    std::size_t count = 0;
    if (DIR* directory = opendir("/proc/self/task"))
    {
        while (const dirent* entry = readdir(directory))
            if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') ++count;
        closedir(directory);
    }
    return count;
}
#endif

double utilization(const CpuTimes& previous, const CpuTimes& current)
{
    if (current.total <= previous.total) return 0.0;
    const std::uint64_t total = current.total - previous.total;
    const std::uint64_t idle = current.idle >= previous.idle ? current.idle - previous.idle : 0;
    return 100.0 * static_cast<double>(total - std::min(total, idle)) / static_cast<double>(total);
}
}

ResourceProfiler::ResourceProfiler(std::chrono::milliseconds interval) : interval_(interval) {}
ResourceProfiler::~ResourceProfiler() { stop(); }

void ResourceProfiler::start()
{
    if (worker_.joinable()) return;
    stopRequested_ = false;
    startedAt_ = std::chrono::steady_clock::now();
    worker_ = std::thread(&ResourceProfiler::run, this);
}

void ResourceProfiler::setPhase(std::string phase)
{
    std::lock_guard<std::mutex> lock(mutex_);
    phase_ = std::move(phase);
}

void ResourceProfiler::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopRequested_ = true;
    }
    if (worker_.joinable()) worker_.join();
}

void ResourceProfiler::run()
{
    auto previousCpus = cpuTimes();
    std::uint64_t previousProcess = processCpuTime();
    auto previousTime = std::chrono::steady_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(interval_);
        std::string phase;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopRequested_) break;
            phase = phase_;
        }
        const auto now = std::chrono::steady_clock::now();
        const auto currentCpus = cpuTimes();
        const std::uint64_t currentProcess = processCpuTime();
        ResourceSample sample;
        sample.elapsedMilliseconds = std::chrono::duration<double, std::milli>(now - startedAt_).count();
        sample.phase = std::move(phase);
        const double elapsedSeconds = std::chrono::duration<double>(now - previousTime).count();
        const unsigned int logicalCpus = std::max(1U, std::thread::hardware_concurrency());
#ifdef _WIN32
        const double processSeconds = static_cast<double>(currentProcess - previousProcess) / 10000000.0;
#else
        const double processSeconds = static_cast<double>(currentProcess - previousProcess) / 1000000.0;
#endif
        if (elapsedSeconds > 0.0)
            sample.processCpuPercentOfTotalCapacity = 100.0 * processSeconds / elapsedSeconds / logicalCpus;
        const std::size_t count = std::min(previousCpus.size(), currentCpus.size());
        for (std::size_t index = 0; index < count; ++index)
            sample.systemCpuPercentPerCore.push_back(utilization(previousCpus[index], currentCpus[index]));
        memory(sample.residentMemoryBytes, sample.privateMemoryBytes);
        sample.threadCount = threadCount();
        samples_.push_back(std::move(sample));
        previousCpus = currentCpus;
        previousProcess = currentProcess;
        previousTime = now;
    }
}

ResourceSummary ResourceProfiler::summarize(const std::string& phase) const
{
    ResourceSummary result;
    double processSum = 0.0, systemSum = 0.0;
    std::size_t coreSamples = 0;
    for (const auto& sample : samples_)
    {
        if (!phase.empty() && sample.phase != phase) continue;
        ++result.samples;
        processSum += sample.processCpuPercentOfTotalCapacity;
        result.processCpuPeak = std::max(result.processCpuPeak, sample.processCpuPercentOfTotalCapacity);
        if (result.residentMemoryMinimumBytes == 0 || sample.residentMemoryBytes < result.residentMemoryMinimumBytes)
            result.residentMemoryMinimumBytes = sample.residentMemoryBytes;
        result.residentMemoryMaximumBytes = std::max(result.residentMemoryMaximumBytes, sample.residentMemoryBytes);
        result.privateMemoryMaximumBytes = std::max(result.privateMemoryMaximumBytes, sample.privateMemoryBytes);
        result.threadCountMaximum = std::max(result.threadCountMaximum, sample.threadCount);
        for (double core : sample.systemCpuPercentPerCore)
        {
            systemSum += core;
            ++coreSamples;
            result.systemCpuPeakCore = std::max(result.systemCpuPeakCore, core);
        }
    }
    if (result.samples) result.processCpuMean = processSum / result.samples;
    if (coreSamples) result.systemCpuMean = systemSum / coreSamples;
    return result;
}

bool ResourceProfiler::writeCsv(const std::filesystem::path& path, std::string& error) const
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) { error = "Cannot write resource trace: " + path.string(); return false; }
    output << "elapsed_ms,phase,process_cpu_percent_total_capacity,resident_memory_bytes,private_memory_bytes,thread_count,system_cpu_percent_per_core\n";
    output << std::fixed;
    for (const auto& sample : samples_)
    {
        output << sample.elapsedMilliseconds << ',' << sample.phase << ','
               << sample.processCpuPercentOfTotalCapacity << ',' << sample.residentMemoryBytes << ','
               << sample.privateMemoryBytes << ',' << sample.threadCount << ",\"";
        for (std::size_t index = 0; index < sample.systemCpuPercentPerCore.size(); ++index)
        {
            if (index) output << ';';
            output << sample.systemCpuPercentPerCore[index];
        }
        output << "\"\n";
    }
    return true;
}
