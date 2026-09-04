#include "ResourceProfiler.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

namespace
{
bool expect(bool condition, const char* label)
{
    if (!condition) std::cerr << "FAILED: " << label << '\n';
    return condition;
}
}

int main()
{
    ResourceProfiler profiler(std::chrono::milliseconds(10));
    profiler.start();
    profiler.setPhase("calibration");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    profiler.setPhase("processing");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    profiler.stop();

    bool ok = true;
    const auto overall = profiler.summarize();
    ok &= expect(overall.samples >= 2, "periodic samples captured");
    ok &= expect(overall.residentMemoryMaximumBytes > 0, "resident memory captured");
    ok &= expect(overall.threadCountMaximum >= 2, "thread count captured");
    ok &= expect(profiler.summarize("calibration").samples > 0, "calibration phase captured");
    ok &= expect(profiler.summarize("processing").samples > 0, "processing phase captured");
    ok &= expect(!profiler.samples().empty() &&
                 !profiler.samples().front().systemCpuPercentPerCore.empty(),
                 "per-core CPU captured");

    const auto output = std::filesystem::temp_directory_path() /
        "face-resource-profiler-test.csv";
    std::string error;
    ok &= expect(profiler.writeCsv(output, error), "CSV written");
    ok &= expect(std::filesystem::file_size(output) > 100, "CSV contains samples");
    std::error_code ignored;
    std::filesystem::remove(output, ignored);

    if (!ok) return 1;
    std::cout << "Resource profiler tests PASSED\n";
    return 0;
}
