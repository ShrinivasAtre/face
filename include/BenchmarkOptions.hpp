#pragma once

#include "BackendOptions.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

struct BenchmarkOptions
{
    BackendKind backend = BackendKind::YuNet;
    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path trace;
    std::filesystem::path eyeCropsDirectory;
    std::filesystem::path pfldModel;
    std::size_t warmupFrames = 10;
    std::size_t measuredFrames = 100;
    std::size_t eyeCropEvery = 6;
    bool sponsorDemo = false;
    bool sponsorDemoAutoExit = false;
    bool showHelp = false;
};

bool parseBenchmarkOptions(int argc, const char* const argv[],
                           BenchmarkOptions& options, std::string& error);
std::string benchmarkUsage(const char* programName);
