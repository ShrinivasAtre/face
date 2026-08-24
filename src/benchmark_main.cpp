#include "AppPaths.hpp"
#include "BackendEyeMapper.hpp"
#include "BenchmarkOptions.hpp"
#include "BlinkTracker.hpp"
#include "FaceBackend.hpp"
#include "YuNetLbfBackend.hpp"

#ifdef FACE_MEDIAPIPE_RUNTIME_ENABLED
#include "MediaPipeBackend.hpp"
#endif

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace
{
using Clock = std::chrono::steady_clock;

struct Samples
{
    std::vector<double> capture;
    std::vector<double> backend;
    std::vector<double> semantic;
    std::vector<double> total;
};

struct Summary
{
    double mean = 0.0;
    double p50 = 0.0;
    double p95 = 0.0;
    double p99 = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
};

double milliseconds(Clock::duration duration)
{
    return std::chrono::duration<double, std::milli>(duration).count();
}

double percentile(const std::vector<double>& sorted, double fraction)
{
    if (sorted.empty()) return 0.0;
    const double position = fraction * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(position));
    const auto upper = static_cast<std::size_t>(std::ceil(position));
    const double weight = position - static_cast<double>(lower);
    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

Summary summarize(std::vector<double> values)
{
    Summary result;
    if (values.empty()) return result;
    std::sort(values.begin(), values.end());
    double sum = 0.0;
    for (double value : values) sum += value;
    result.mean = sum / static_cast<double>(values.size());
    result.minimum = values.front();
    result.maximum = values.back();
    result.p50 = percentile(values, 0.50);
    result.p95 = percentile(values, 0.95);
    result.p99 = percentile(values, 0.99);
    return result;
}

std::uint64_t currentResidentMemoryBytes()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        return static_cast<std::uint64_t>(counters.WorkingSetSize);
    }
    return 0;
#else
    std::ifstream statm("/proc/self/statm");
    std::uint64_t totalPages = 0;
    std::uint64_t residentPages = 0;
    if (statm >> totalPages >> residentPages)
    {
        const long pageSize = sysconf(_SC_PAGESIZE);
        if (pageSize > 0)
        {
            return residentPages * static_cast<std::uint64_t>(pageSize);
        }
    }
    return 0;
#endif
}

std::uint64_t peakResidentMemoryBytes()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
    }
    return 0;
#else
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
#if defined(__APPLE__)
    const std::uint64_t reportedPeak = static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    const std::uint64_t reportedPeak =
        static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
    return std::max(reportedPeak, currentResidentMemoryBytes());
#endif
}

const char* buildConfiguration() noexcept
{
#ifdef NDEBUG
    return "Release";
#else
    return "Debug";
#endif
}

std::string jsonEscape(const std::string& input)
{
    std::ostringstream output;
    for (const unsigned char character : input)
    {
        switch (character)
        {
        case '\\': output << "\\\\"; break;
        case '"': output << "\\\""; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (character < 0x20)
            {
                output << "\\u" << std::hex << std::setw(4)
                       << std::setfill('0') << static_cast<int>(character)
                       << std::dec << std::setfill(' ');
            }
            else output << character;
        }
    }
    return output.str();
}

void writeSummary(std::ostream& output, const char* name,
                  const Summary& value, bool trailingComma)
{
    output << "    \"" << name << "\": {"
           << "\"mean\": " << value.mean
           << ", \"p50\": " << value.p50
           << ", \"p95\": " << value.p95
           << ", \"p99\": " << value.p99
           << ", \"min\": " << value.minimum
           << ", \"max\": " << value.maximum << "}"
           << (trailingComma ? "," : "") << '\n';
}

class InputFrames
{
public:
    bool open(const std::filesystem::path& path)
    {
        image_ = cv::imread(path.string(), cv::IMREAD_COLOR);
        if (!image_.empty())
        {
            kind_ = "image-repeat";
            return true;
        }
        capture_.open(path.string());
        if (!capture_.isOpened()) return false;
        kind_ = "video";
        return true;
    }

    bool next(cv::Mat& frame)
    {
        if (!image_.empty())
        {
            frame = image_;
            return true;
        }
        if (capture_.read(frame) && !frame.empty()) return true;
        capture_.set(cv::CAP_PROP_POS_FRAMES, 0.0);
        return capture_.read(frame) && !frame.empty();
    }

    const char* kind() const noexcept { return kind_; }

private:
    cv::Mat image_;
    cv::VideoCapture capture_;
    const char* kind_ = "unknown";
};
}

int main(int argc, char** argv)
{
    BenchmarkOptions options;
    std::string error;
    if (!parseBenchmarkOptions(argc, argv, options, error))
    {
        std::cerr << "Error: " << error << '\n'
                  << benchmarkUsage(argc > 0 ? argv[0] : nullptr) << '\n';
        return 2;
    }
    if (options.showHelp)
    {
        std::cout << benchmarkUsage(argc > 0 ? argv[0] : nullptr) << '\n';
        return 0;
    }
#ifndef FACE_MEDIAPIPE_RUNTIME_ENABLED
    if (options.backend == BackendKind::MediaPipe)
    {
        std::cerr << "Error: MediaPipe backend is unavailable in this build.\n";
        return 2;
    }
#endif

    try
    {
        InputFrames input;
        if (!input.open(options.input))
        {
            std::cerr << "Error: Cannot open benchmark input: "
                      << options.input << '\n';
            return 1;
        }

        const std::filesystem::path executableDir = AppPaths::executableDirectory();
        const std::filesystem::path modelDir = executableDir / "models";
        std::unique_ptr<FaceBackend> backend;
        std::string modelPath;
        if (options.backend == BackendKind::YuNet)
        {
            backend = std::make_unique<YuNetLbfBackend>(
                (modelDir / "lbfmodel.yaml").string());
            modelPath = (modelDir / "face_detection_yunet_2026may.onnx").string();
        }
#ifdef FACE_MEDIAPIPE_RUNTIME_ENABLED
        else
        {
#ifdef _WIN32
            const auto libraryPath = executableDir / "FaceMediaPipe.dll";
#else
            const auto libraryPath = executableDir / "libFaceMediaPipe.so";
#endif
            backend = std::make_unique<MediaPipeBackend>(libraryPath);
            modelPath = (modelDir / "mediapipe" / "face_landmarker.task").string();
        }
#endif
        if (!backend || !backend->initialize(modelPath))
        {
            std::cerr << "Error: Backend initialization failed.\n";
            return 1;
        }

        BlinkTracker tracker(0.27);
        cv::Mat frame;
        FaceResult faceResult;
        Samples samples;
        samples.capture.reserve(options.measuredFrames);
        samples.backend.reserve(options.measuredFrames);
        samples.semantic.reserve(options.measuredFrames);
        samples.total.reserve(options.measuredFrames);
        std::size_t detectedFrames = 0;
        std::size_t successfulFrames = 0;
        const std::uint64_t initialResidentMemory = currentResidentMemoryBytes();

        const std::size_t totalFrames = options.warmupFrames + options.measuredFrames;
        const std::clock_t cpuStart = std::clock();
        const auto wallStart = Clock::now();
        for (std::size_t index = 0; index < totalFrames; ++index)
        {
            const auto totalStart = Clock::now();
            const auto captureStart = totalStart;
            if (!input.next(frame))
            {
                std::cerr << "Error: Input ended and could not restart.\n";
                return 1;
            }
            const auto captureEnd = Clock::now();
            const auto backendStart = captureEnd;
            const bool backendSuccess = backend->process(frame, faceResult);
            const auto backendEnd = Clock::now();
            const auto semanticStart = backendEnd;
            bool semanticSuccess = false;
            if (backendSuccess && faceResult.detected)
            {
                SemanticEyeLandmarks eyes;
                if (mapBackendEyeLandmarks(options.backend, faceResult, eyes))
                {
                    semanticSuccess = tracker.process(frame, eyes);
                }
            }
            const auto semanticEnd = Clock::now();

            if (index >= options.warmupFrames)
            {
                samples.capture.push_back(milliseconds(captureEnd - captureStart));
                samples.backend.push_back(milliseconds(backendEnd - backendStart));
                samples.semantic.push_back(milliseconds(semanticEnd - semanticStart));
                samples.total.push_back(milliseconds(semanticEnd - totalStart));
                if (backendSuccess) ++successfulFrames;
                if (backendSuccess && faceResult.detected) ++detectedFrames;
                (void)semanticSuccess;
            }
        }
        const auto wallEnd = Clock::now();
        const std::clock_t cpuEnd = std::clock();
        const double wallSeconds = std::chrono::duration<double>(wallEnd - wallStart).count();
        const double measuredSeconds = [&]() {
            double sum = 0.0;
            for (double value : samples.total) sum += value;
            return sum / 1000.0;
        }();
        const unsigned int logicalCpus = std::max(1U, std::thread::hardware_concurrency());
        const double cpuSeconds = static_cast<double>(cpuEnd - cpuStart) / CLOCKS_PER_SEC;
        const double cpuPercentCapacity = wallSeconds > 0.0
            ? 100.0 * cpuSeconds / wallSeconds / logicalCpus : 0.0;
        const std::uint64_t finalResidentMemory = currentResidentMemoryBytes();
        const std::int64_t residentMemoryGrowth =
            static_cast<std::int64_t>(finalResidentMemory) -
            static_cast<std::int64_t>(initialResidentMemory);

        std::ostringstream json;
        json << std::fixed << std::setprecision(6)
             << "{\n"
             << "  \"schema_version\": 2,\n"
             << "  \"backend\": \"" << backendName(options.backend) << "\",\n"
             << "  \"build_configuration\": \"" << buildConfiguration() << "\",\n"
             << "  \"input\": \"" << jsonEscape(options.input.string()) << "\",\n"
             << "  \"input_kind\": \"" << input.kind() << "\",\n"
             << "  \"input_width\": " << frame.cols << ",\n"
             << "  \"input_height\": " << frame.rows << ",\n"
             << "  \"warmup_frames\": " << options.warmupFrames << ",\n"
             << "  \"measured_frames\": " << options.measuredFrames << ",\n"
             << "  \"successful_frames\": " << successfulFrames << ",\n"
             << "  \"detected_frames\": " << detectedFrames << ",\n"
             << "  \"dropped_frames\": 0,\n"
             << "  \"superseded_frames\": 0,\n"
             << "  \"rendered_frames\": 0,\n"
             << "  \"throughput_fps\": "
             << (measuredSeconds > 0.0 ? options.measuredFrames / measuredSeconds : 0.0)
             << ",\n"
             << "  \"process_cpu_percent_of_total_capacity\": "
             << cpuPercentCapacity << ",\n"
             << "  \"logical_cpu_count\": " << logicalCpus << ",\n"
             << "  \"initial_resident_memory_bytes\": " << initialResidentMemory << ",\n"
             << "  \"final_resident_memory_bytes\": " << finalResidentMemory << ",\n"
             << "  \"resident_memory_growth_bytes\": " << residentMemoryGrowth << ",\n"
             << "  \"peak_resident_memory_bytes\": " << peakResidentMemoryBytes() << ",\n"
             << "  \"latency_ms\": {\n";
        writeSummary(json, "capture", summarize(samples.capture), true);
        writeSummary(json, "backend", summarize(samples.backend), true);
        writeSummary(json, "semantic", summarize(samples.semantic), true);
        writeSummary(json, "end_to_end", summarize(samples.total), false);
        json << "  }\n}\n";

        if (!options.output.empty())
        {
            std::ofstream file(options.output, std::ios::binary | std::ios::trunc);
            if (!file)
            {
                std::cerr << "Error: Cannot write benchmark output: "
                          << options.output << '\n';
                return 1;
            }
            file << json.str();
        }
        std::cout << json.str();
        return successfulFrames == options.measuredFrames ? 0 : 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal benchmark error: " << exception.what() << '\n';
        return 1;
    }
}
