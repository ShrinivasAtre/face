#include "AppPaths.hpp"
#include "BackendEyeMapper.hpp"
#include "BackendFaceGeometryMapper.hpp"
#include "BackendGazeMapper.hpp"
#include "BenchmarkOptions.hpp"
#include "BlinkTracker.hpp"
#include "DmsEyeMetrics.hpp"
#include "DmsEyeQualityAssessor.hpp"
#include "DmsHeadPoseEstimator.hpp"
#include "DmsTemporalEvents.hpp"
#include "DmsPolicy.hpp"
#include "EyeCropExtractor.hpp"
#include "FaceBackend.hpp"
#include "PfldEyeLandmarkMapper.hpp"
#include "RecordedFrameClock.hpp"
#include "ResourceProfiler.hpp"
#include "YuNetLbfBackend.hpp"
#include "YuNetPfldBackend.hpp"

#ifdef FACE_MEDIAPIPE_RUNTIME_ENABLED
#include "MediaPipeBackend.hpp"
#endif

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
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

#ifndef FACE_SOURCE_REVISION
#define FACE_SOURCE_REVISION "unknown"
#endif

struct Samples
{
    std::vector<double> capture;
    std::vector<double> backend;
    std::vector<double> semantic;
    std::vector<double> total;
    std::vector<double> faceGeometry;
    std::vector<double> eyeMappingAndEar;
    std::vector<double> eyeQualityAndCalibration;
    std::vector<double> temporalFsms;
    std::vector<double> output;
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

double percentile(const std::vector<double> &sorted, double fraction)
{
    if (sorted.empty())
        return 0.0;
    const double position = fraction * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(position));
    const auto upper = static_cast<std::size_t>(std::ceil(position));
    const double weight = position - static_cast<double>(lower);
    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

Summary summarize(std::vector<double> values)
{
    Summary result;
    if (values.empty())
        return result;
    std::sort(values.begin(), values.end());
    double sum = 0.0;
    for (double value : values)
        sum += value;
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
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        return 0;
#if defined(__APPLE__)
    const std::uint64_t reportedPeak = static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    const std::uint64_t reportedPeak = static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
    return std::max(reportedPeak, currentResidentMemoryBytes());
#endif
}

const char *buildConfiguration() noexcept
{
#ifdef NDEBUG
    return "Release";
#else
    return "Debug";
#endif
}

const char *platformName() noexcept
{
#ifdef _WIN32
    return "windows-x64";
#elif defined(__aarch64__)
    return "linux-aarch64";
#elif defined(__x86_64__)
    return "linux-x64";
#else
    return "unknown";
#endif
}

const char *compilerName() noexcept
{
#ifdef _MSC_VER
    return "msvc";
#elif defined(__clang__)
    return "clang-" __clang_version__;
#elif defined(__GNUC__)
    return "gcc-" __VERSION__;
#else
    return "unknown";
#endif
}

std::string jsonEscape(const std::string &input)
{
    std::ostringstream output;
    for (const unsigned char character : input)
    {
        switch (character)
        {
        case '\\':
            output << "\\\\";
            break;
        case '"':
            output << "\\\"";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (character < 0x20)
            {
                output << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(character)
                       << std::dec << std::setfill(' ');
            }
            else
                output << character;
        }
    }
    return output.str();
}

void writeSummary(std::ostream &output, const char *name, const Summary &value, bool trailingComma)
{
    output << "    \"" << name << "\": {"
           << "\"mean\": " << value.mean << ", \"p50\": " << value.p50 << ", \"p95\": " << value.p95
           << ", \"p99\": " << value.p99 << ", \"min\": " << value.minimum << ", \"max\": " << value.maximum << "}"
           << (trailingComma ? "," : "") << '\n';
}

void writeResourceSummary(std::ostream& output, const char* name,
                          const ResourceSummary& value, bool trailingComma)
{
    output << "    \"" << name << "\": {\"samples\": " << value.samples
           << ", \"process_cpu_mean_percent_total_capacity\": " << value.processCpuMean
           << ", \"process_cpu_peak_percent_total_capacity\": " << value.processCpuPeak
           << ", \"system_cpu_mean_percent\": " << value.systemCpuMean
           << ", \"system_cpu_peak_core_percent\": " << value.systemCpuPeakCore
           << ", \"resident_memory_min_bytes\": " << value.residentMemoryMinimumBytes
           << ", \"resident_memory_max_bytes\": " << value.residentMemoryMaximumBytes
           << ", \"private_memory_max_bytes\": " << value.privateMemoryMaximumBytes
           << ", \"thread_count_max\": " << value.threadCountMaximum << "}"
           << (trailingComma ? "," : "") << '\n';
}

const char *eyeStateName(dms::EyeState state) noexcept
{
    switch (state)
    {
    case dms::EyeState::Open:
        return "open";
    case dms::EyeState::Closed:
        return "closed";
    case dms::EyeState::Unknown:
        return "unknown";
    }
    return "unknown";
}

const char *usabilityName(dms::ObservationUsability usability) noexcept
{
    switch (usability)
    {
    case dms::ObservationUsability::Usable:
        return "usable";
    case dms::ObservationUsability::Recovering:
        return "recovering";
    case dms::ObservationUsability::Missing:
        return "missing";
    case dms::ObservationUsability::LowConfidence:
        return "low_confidence";
    case dms::ObservationUsability::Occluded:
        return "occluded";
    case dms::ObservationUsability::Stale:
        return "stale";
    case dms::ObservationUsability::FutureTimestamp:
        return "future_timestamp";
    }
    return "missing";
}

const char *headZoneName(dms::HeadZone zone) noexcept
{
    switch (zone)
    {
    case dms::HeadZone::Neutral: return "neutral";
    case dms::HeadZone::Left: return "left";
    case dms::HeadZone::Right: return "right";
    case dms::HeadZone::Up: return "up";
    case dms::HeadZone::Down: return "down";
    default: return "unknown";
    }
}

const char *gazeZoneName(dms::GazeZone zone) noexcept
{
    switch (zone)
    {
    case dms::GazeZone::Forward: return "forward";
    case dms::GazeZone::Left: return "left";
    case dms::GazeZone::Right: return "right";
    case dms::GazeZone::Up: return "up";
    case dms::GazeZone::Down: return "down";
    default: return "unknown";
    }
}

const char *presenceName(dms::PresenceState state) noexcept
{
    switch (state)
    {
    case dms::PresenceState::Present: return "present";
    case dms::PresenceState::Absent: return "absent";
    default: return "unknown";
    }
}

const char *drowsinessName(dms::DrowsinessState state) noexcept
{
    switch (state)
    {
    case dms::DrowsinessState::Normal: return "normal";
    case dms::DrowsinessState::Warning: return "warning";
    case dms::DrowsinessState::Drowsy: return "drowsy";
    default: return "unknown";
    }
}

std::string optionalPercent(std::optional<float> value)
{
    return value ? cv::format("%.1f%%", *value * 100.0F) : "N/A";
}

void drawSponsorOverlay(cv::Mat& frame, BackendKind backend, dms::MonotonicTime timestamp,
                        dms::ObservationUsability eyeUsability,
                        const dms::EyeMetricResult& eye, const dms::YawnResult& yawn,
                        const dms::HeadPoseResult& head, const dms::DistractionResult& distraction,
                        dms::PresenceState presence,
                        const dms::MonitoringAvailabilityResult& availability,
                        const dms::DrowsinessResult& drowsiness, double processingFps,
                        bool gazeAvailable, std::uint64_t distractionCount, bool complete)
{
    const int panelWidth = std::min(780, frame.cols - 20);
    const int panelHeight = std::min(330, frame.rows - 20);
    cv::Mat panel = frame(cv::Rect(10, 10, panelWidth, panelHeight));
    cv::Mat shade(panel.size(), panel.type(), cv::Scalar(18, 18, 18));
    cv::addWeighted(shade, 0.72, panel, 0.28, 0.0, panel);
    const cv::Scalar normal(240, 240, 240), good(80, 230, 100), warn(40, 190, 255), bad(70, 70, 255);
    std::vector<std::pair<std::string, cv::Scalar>> lines;
    lines.push_back({complete ? "DMS ENGINEERING DEMO - VIDEO COMPLETE"
                              : "DMS ENGINEERING DEMO - DEVELOPMENT BUILD", complete ? good : normal});
    lines.push_back({std::string("Backend: ") + backendName(backend) + "  Time: " +
                     cv::format("%.2fs", std::chrono::duration<double>(timestamp).count()) +
                     "  Processing: " + cv::format("%.1f FPS", processingFps), normal});
    lines.push_back({"Driver: " + std::string(presenceName(presence)) +
                     "  Monitoring: " + (availability.unavailable ? "UNAVAILABLE" : "available"),
                     availability.notify ? bad : (availability.unavailable ? warn : good)});
    lines.push_back({"Eyes: " + std::string(eyeStateName(eye.state)) +
                     "  Openness: " + optionalPercent(eye.openness) +
                     "  Quality: " + usabilityName(eyeUsability),
                     eye.state == dms::EyeState::Closed ? warn : normal});
    lines.push_back({"Blinks: " + std::to_string(eye.blinkCount) +
                     "  Long: " + std::to_string(eye.longBlinkCount) +
                     "  Prolonged: " + std::to_string(eye.prolongedClosureCount) +
                     "  PERCLOS: " + optionalPercent(eye.perclos),
                     eye.prolongedClosure ? bad : normal});
    lines.push_back({"Yawns: " + std::to_string(yawn.count) +
                     "  Head: " + headZoneName(head.zone) +
                     "  L/R/U/D: " + std::to_string(head.leftCount) + "/" +
                     std::to_string(head.rightCount) + "/" + std::to_string(head.upCount) + "/" +
                     std::to_string(head.downCount), yawn.active ? warn : normal});
    lines.push_back({"Gaze: " + std::string(gazeAvailable ? gazeZoneName(distraction.gaze) : "N/A for provider") +
                     "  Distraction: " + (distraction.distracted ? "YES" : "no") +
                     " (" + std::to_string(distractionCount) + ")" +
                     "  Drowsiness: " + drowsinessName(drowsiness.state),
                     drowsiness.state == dms::DrowsinessState::Drowsy ? bad :
                         (drowsiness.state == dms::DrowsinessState::Warning ? warn : normal)});
    std::string event = "Event: ";
    if (eye.prolongedClosureEvent) event += "PROLONGED CLOSURE";
    else if (eye.longBlinkEvent) event += "LONG BLINK";
    else if (eye.blinkEvent) event += "BLINK";
    else if (yawn.event) event += "YAWN";
    else if (head.movementEvent) event += "HEAD MOVEMENT";
    else if (distraction.event) event += "DISTRACTION";
    else event += "-";
    lines.push_back({event, event == "Event: -" ? normal : warn});
    int y = 37;
    for (const auto& line : lines)
    {
        cv::putText(frame, line.first, {25, y}, cv::FONT_HERSHEY_SIMPLEX, 0.62,
                    cv::Scalar(0, 0, 0), 4, cv::LINE_AA);
        cv::putText(frame, line.first, {25, y}, cv::FONT_HERSHEY_SIMPLEX, 0.62,
                    line.second, 1, cv::LINE_AA);
        y += 37;
    }
}

class InputFrames
{
  public:
    bool open(const std::filesystem::path &path)
    {
        if (std::filesystem::is_directory(path))
        {
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_regular_file())
                    sequence_.push_back(entry.path());
            }
            std::sort(sequence_.begin(), sequence_.end());
            if (sequence_.empty())
                return false;
            cv::Mat probe = cv::imread(sequence_.front().string(), cv::IMREAD_COLOR);
            if (probe.empty())
                return false;
            kind_ = "image-sequence";
            clock_.reset(30.0);
            return true;
        }
        image_ = cv::imread(path.string(), cv::IMREAD_COLOR);
        if (!image_.empty())
        {
            kind_ = "image-repeat";
            clock_.reset(30.0);
            return true;
        }
        // Prefer the same software decoder on every benchmark target. Orin's
        // default GStreamer selection can route ordinary MP4 input through a
        // hardware decoder that rejects otherwise valid recordings.
        capture_.open(path.string(), cv::CAP_FFMPEG);
#ifndef _WIN32
        if (!capture_.isOpened())
        {
            std::string escapedPath;
            for (const char character : path.string())
            {
                if (character == '\\' || character == '"') escapedPath.push_back('\\');
                escapedPath.push_back(character);
            }
            const std::string softwareH264Pipeline =
                "filesrc location=\"" + escapedPath +
                "\" ! qtdemux ! h264parse ! avdec_h264 ! videoconvert "
                "! video/x-raw,format=BGR ! appsink sync=false";
            capture_.open(softwareH264Pipeline, cv::CAP_GSTREAMER);
        }
#endif
        if (!capture_.isOpened())
            capture_.open(path.string());
        if (!capture_.isOpened())
            return false;
        kind_ = "video";
        clock_.reset(capture_.get(cv::CAP_PROP_FPS));
        return true;
    }

    bool next(cv::Mat &frame, bool restartAtEnd = true)
    {
        if (!sequence_.empty())
        {
            frame = cv::imread(sequence_[sequenceIndex_].string(), cv::IMREAD_COLOR);
            if (frame.empty())
                return false;
            sequenceIndex_ = (sequenceIndex_ + 1) % sequence_.size();
            clock_.advance(std::nullopt);
            return true;
        }
        if (!image_.empty())
        {
            frame = image_;
            clock_.advance(std::nullopt);
            return true;
        }
        if (capture_.read(frame) && !frame.empty())
        {
            clock_.advance(capture_.get(cv::CAP_PROP_POS_MSEC));
            return true;
        }
        if (!restartAtEnd) return false;
        capture_.set(cv::CAP_PROP_POS_FRAMES, 0.0);
        if (!capture_.read(frame) || frame.empty())
            return false;
        clock_.advance(capture_.get(cv::CAP_PROP_POS_MSEC));
        return true;
    }

    const char *kind() const noexcept
    {
        return kind_;
    }

    dms::MonotonicTime timestamp() const noexcept
    {
        return clock_.timestamp();
    }

    bool isVideo() const noexcept { return capture_.isOpened(); }
    double framesPerSecond() const noexcept
    {
        const double fps = capture_.isOpened() ? capture_.get(cv::CAP_PROP_FPS) : 30.0;
        return std::isfinite(fps) && fps > 0.0 ? fps : 30.0;
    }

  private:
    cv::Mat image_;
    std::vector<std::filesystem::path> sequence_;
    std::size_t sequenceIndex_ = 0;
    cv::VideoCapture capture_;
    dms::RecordedFrameClock clock_;
    const char *kind_ = "unknown";
};
} // namespace

int main(int argc, char **argv)
{
    BenchmarkOptions options;
    std::string error;
    if (!parseBenchmarkOptions(argc, argv, options, error))
    {
        std::cerr << "Error: " << error << '\n' << benchmarkUsage(argc > 0 ? argv[0] : nullptr) << '\n';
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
        ResourceProfiler resourceProfiler(
            std::chrono::milliseconds(options.resourceSampleMilliseconds));
        if (options.resourceProfile)
        {
            resourceProfiler.start();
            resourceProfiler.setPhase("startup");
        }
        InputFrames input;
        if (!input.open(options.input))
        {
            std::cerr << "Error: Cannot open benchmark input: " << options.input << '\n';
            return 1;
        }
        if (options.sponsorDemo && !input.isVideo())
        {
            std::cerr << "Error: --sponsor-demo requires a video file input.\n";
            return 1;
        }

        const std::filesystem::path executableDir = AppPaths::executableDirectory();
        const std::filesystem::path modelDir = executableDir / "models";
        std::unique_ptr<FaceBackend> backend;
        std::string modelPath;
        if (options.backend == BackendKind::YuNet)
        {
            backend = std::make_unique<YuNetLbfBackend>((modelDir / "lbfmodel.yaml").string());
            modelPath = (modelDir / "face_detection_yunet_2026may.onnx").string();
        }
        else if (options.backend == BackendKind::Pfld)
        {
            backend = std::make_unique<YuNetPfldBackend>(options.pfldModel.string());
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
        const dms::OperationalPolicyProfile policy = dms::OperationalPolicyProfile::stage20Approved();
        std::string policyError;
        if (!policy.validate(policyError)) { std::cerr << "Error: " << policyError << '\n'; return 1; }
        dms::EyeTemporalMetrics eyeMetrics(policy.eyeCalibration, policy.eye);
        dms::EyeOpenCalibrator eyeCalibrator(policy.eyeOpenCalibration);
        bool eyeCalibrationApplied = false;
        bool calibrationCompletedOnce = false;
        dms::ObservationQualityGate qualityGate(policy.eyeQuality);
        EyeQualityAssessor eyeQualityAssessor;
        dms::YawnFsm yawnFsm(policy.yawn);
        dms::HeadPoseFsm headPoseFsm(policy.headPose);
        HeadPoseNeutralConfig neutralConfig;
        neutralConfig.confirmation = policy.neutralCalibration;
        HeadPoseNeutralCalibrator headPoseCalibrator(neutralConfig);
        dms::DistractionFsm distractionFsm(policy.distraction);
        dms::DriverPresenceFsm presenceFsm(policy.presence);
        dms::MonitoringAvailabilityFsm availabilityFsm(policy.availability);
        dms::DrowsinessFsm drowsinessFsm(policy.drowsiness);
        if (!eyeMetrics.valid())
        {
            std::cerr << "Error: Invalid eye metric configuration: " << eyeMetrics.error() << '\n';
            return 1;
        }
        if (!qualityGate.valid() || !eyeQualityAssessor.valid() || !yawnFsm.valid() ||
            !headPoseFsm.valid() || !distractionFsm.valid())
        {
            std::cerr << "Error: Invalid semantic metric/FSM configuration.\n";
            return 1;
        }
        std::ofstream trace;
        if (!options.trace.empty())
        {
            trace.open(options.trace, std::ios::binary | std::ios::trunc);
            if (!trace)
            {
                std::cerr << "Error: Cannot write benchmark trace: " << options.trace << '\n';
                return 1;
            }
            trace << "frame,timestamp_ms,backend_success,detected,landmarks_valid,"
                     "semantic_valid,right_ear,left_ear,average_ear,eye_closed,"
                     "blink_count,eye_usability,eye_openness,temporal_eye_state,"
                     "temporal_blink_event,temporal_blink_count,long_blink_event,long_blink_count,closure_ms,"
                     "prolonged_closure,prolonged_closure_event,prolonged_closure_count,perclos,perclos_coverage,"
                     "eye_quality_confidence,eye_visibility,eye_mean,eye_contrast,eye_laplacian,"
                     "mouth_openness,yawn_active,yawn_event,yawn_count,"
                     "yaw_degrees,pitch_degrees,pose_reprojection,head_zone,head_event,"
                     "left_count,right_count,up_count,down_count,"
                     "gaze_horizontal,gaze_vertical,gaze_agreement,gaze_zone,"
                     "distracted,distraction_event,presence,monitoring_unavailable,monitoring_notify,"
                     "monitoring_episode_count,drowsiness,recent_yawns,"
                     "face_x,face_y,face_width,face_height,"
                     "backend_ms,end_to_end_ms\n";
            trace << std::fixed << std::setprecision(6);
        }
        std::ofstream eyeCropManifest;
        std::filesystem::path rightEyeCropDirectory;
        std::filesystem::path leftEyeCropDirectory;
        std::size_t eyeCropPairsWritten = 0;
        std::size_t eyeCropFailures = 0;
        if (!options.eyeCropsDirectory.empty())
        {
            rightEyeCropDirectory = options.eyeCropsDirectory / "right";
            leftEyeCropDirectory = options.eyeCropsDirectory / "left";
            std::error_code directoryError;
            std::filesystem::create_directories(rightEyeCropDirectory, directoryError);
            if (!directoryError)
                std::filesystem::create_directories(leftEyeCropDirectory, directoryError);
            if (directoryError)
            {
                std::cerr << "Error: Cannot create eye-crop directory: "
                          << options.eyeCropsDirectory << " (" << directoryError.message() << ")\n";
                return 1;
            }
            eyeCropManifest.open(options.eyeCropsDirectory / "manifest.csv",
                                 std::ios::binary | std::ios::trunc);
            if (!eyeCropManifest)
            {
                std::cerr << "Error: Cannot write eye-crop manifest in: "
                          << options.eyeCropsDirectory << '\n';
                return 1;
            }
            eyeCropManifest << "frame,timestamp_ms,side,relative_path,width,height\n"
                            << std::fixed << std::setprecision(3);
        }
        cv::Mat frame;
        cv::Size inputSize;
        FaceResult faceResult;
        Samples samples;
        samples.capture.reserve(options.measuredFrames);
        samples.backend.reserve(options.measuredFrames);
        samples.semantic.reserve(options.measuredFrames);
        samples.total.reserve(options.measuredFrames);
        samples.faceGeometry.reserve(options.measuredFrames);
        samples.eyeMappingAndEar.reserve(options.measuredFrames);
        samples.eyeQualityAndCalibration.reserve(options.measuredFrames);
        samples.temporalFsms.reserve(options.measuredFrames);
        samples.output.reserve(options.measuredFrames);
        std::size_t detectedFrames = 0;
        std::size_t successfulFrames = 0;
        std::size_t usableEyeFrames = 0;
        std::size_t unknownEyeFrames = 0;
        std::size_t prolongedClosureFrames = 0;
        std::size_t lowQualityEyeFrames = 0;
        std::size_t occludedEyeFrames = 0;
        std::size_t distractedFrames = 0;
        std::uint64_t distractionEventCount = 0;
        std::size_t monitoringUnavailableFrames = 0, monitoringNotifyFrames = 0;
        std::size_t renderedFrames = 0;
        std::size_t measuredFrames = 0;
        dms::EyeMetricResult finalEyeMetrics;
        dms::YawnResult finalYawn;
        dms::HeadPoseResult finalHeadPose;
        dms::DistractionResult finalDistraction;
        dms::PresenceState finalPresence = dms::PresenceState::Unknown;
        dms::DrowsinessResult finalDrowsiness;
        dms::MonitoringAvailabilityResult finalAvailability;
        dms::ObservationUsability finalEyeUsability = dms::ObservationUsability::Missing;
        bool finalGazeAvailable = false;
        bool userStoppedDemo = false;
        cv::Mat finalDisplayFrame;
        std::optional<dms::MonotonicTime> absentSince;
        std::optional<dms::MonotonicTime> invalidPoseSince;
        const std::uint64_t initialResidentMemory = currentResidentMemoryBytes();

        const std::size_t totalFrames = options.sponsorDemo
            ? std::numeric_limits<std::size_t>::max()
            : options.warmupFrames + options.measuredFrames;
        const std::clock_t cpuStart = std::clock();
        const auto wallStart = Clock::now();
        for (std::size_t index = 0; index < totalFrames; ++index)
        {
            if (index < options.warmupFrames)
                resourceProfiler.setPhase("warmup");
            else if (!eyeCalibrationApplied)
                resourceProfiler.setPhase(calibrationCompletedOnce ? "recalibration" : "calibration");
            else
                resourceProfiler.setPhase("processing");
            const auto totalStart = Clock::now();
            const auto captureStart = totalStart;
            if (!input.next(frame, !options.sponsorDemo))
            {
                if (options.sponsorDemo) break;
                std::cerr << "Error: Input ended and could not restart.\n";
                return 1;
            }
            const auto captureEnd = Clock::now();
            inputSize = frame.size();
            const auto backendStart = captureEnd;
            const bool backendSuccess = backend->process(frame, faceResult);
            const auto backendEnd = Clock::now();
            const auto semanticStart = backendEnd;
            bool semanticSuccess = false;
            SemanticEyeLandmarks eyes;
            bool eyeMapped = false;
            SemanticFaceGeometry faceGeometry;
            const auto faceGeometryStart = Clock::now();
            const bool faceGeometryValid = mapBackendFaceGeometry(options.backend, faceResult, faceGeometry);
            const std::optional<float> mouthOpenness = faceGeometryValid
                ? calculateMouthOpenness(faceGeometry) : std::nullopt;
            HeadPoseAngles poseAngles;
            const bool poseValid = faceGeometryValid &&
                estimateHeadPose(faceGeometry, frame.size(), poseAngles);
            SemanticGaze gaze;
            const bool gazeValid = mapBackendGaze(options.backend, faceResult, gaze);
            const auto faceGeometryEnd = Clock::now();
            const auto eyeMappingStart = faceGeometryEnd;
            if (backendSuccess && faceResult.detected)
            {
                eyeMapped = options.backend == BackendKind::Pfld
                                ? mapPfldEyeLandmarks(faceResult.landmarks, eyes)
                                : mapBackendEyeLandmarks(options.backend, faceResult, eyes);
                if (eyeMapped)
                {
                    if (eyeCropManifest.is_open() && index >= options.warmupFrames)
                    {
                        const std::size_t measuredFrame = index - options.warmupFrames;
                        if (measuredFrame % options.eyeCropEvery == 0)
                        {
                            const auto writeEyeCrop = [&](const char* side,
                                                          const EyeLandmarks& eye,
                                                          bool mirror,
                                                          const std::filesystem::path& directory) {
                                cv::Mat crop;
                                if (!extractAlignedEyeCrop(frame, eye, mirror, crop))
                                {
                                    ++eyeCropFailures;
                                    return false;
                                }
                                std::ostringstream filename;
                                filename << "frame_" << std::setw(6) << std::setfill('0')
                                         << measuredFrame << ".png";
                                const std::filesystem::path outputPath = directory / filename.str();
                                if (!cv::imwrite(outputPath.string(), crop))
                                {
                                    ++eyeCropFailures;
                                    return false;
                                }
                                const std::filesystem::path relative =
                                    std::filesystem::relative(outputPath, options.eyeCropsDirectory);
                                eyeCropManifest << measuredFrame << ','
                                                << std::chrono::duration<double, std::milli>(input.timestamp()).count()
                                                << ',' << side << ',' << relative.generic_string() << ','
                                                << crop.cols << ',' << crop.rows << '\n';
                                return true;
                            };
                            const bool rightWritten = writeEyeCrop(
                                "right", eyes.rightEye, false, rightEyeCropDirectory);
                            const bool leftWritten = writeEyeCrop(
                                "left", eyes.leftEye, false, leftEyeCropDirectory);
                            if (rightWritten && leftWritten)
                                ++eyeCropPairsWritten;
                        }
                    }
                    semanticSuccess = tracker.process(frame, eyes);
                }
            }
            const auto eyeMappingEnd = Clock::now();
            const auto semanticEnd = Clock::now();

            if (index >= options.warmupFrames)
            {
                const auto qualityCalibrationStart = Clock::now();
                dms::EyeMetricInput eyeInput;
                eyeInput.timestamp = input.timestamp();
                if (!faceResult.detected)
                {
                    if (!absentSince) absentSince = input.timestamp();
                    if (input.timestamp() - *absentSince >= policy.recalibrateAfterAbsence)
                    {
                        headPoseCalibrator.reset();
                        eyeCalibrator.reset();
                        eyeMetrics.reset();
                        eyeCalibrationApplied = false;
                    }
                }
                else absentSince.reset();
                if (!poseValid)
                {
                    if (!invalidPoseSince) invalidPoseSince = input.timestamp();
                    if (input.timestamp() - *invalidPoseSince >= policy.recalibrateAfterInvalidGeometry)
                    {
                        headPoseCalibrator.reset();
                        eyeCalibrator.reset();
                        eyeMetrics.reset();
                        eyeCalibrationApplied = false;
                    }
                }
                else invalidPoseSince.reset();
                const auto calibratedPose = poseValid
                    ? headPoseCalibrator.update(input.timestamp(), poseAngles) : std::nullopt;
                dms::ObservationHeader qualityHeader;
                qualityHeader.source = {static_cast<std::uint64_t>(index), input.timestamp()};
                qualityHeader.producedAt = input.timestamp();
                const EyeQualityResult eyeQuality = eyeMapped
                    ? eyeQualityAssessor.assess(frame, eyes) : EyeQualityResult{};
                qualityHeader.validity = semanticSuccess
                    ? eyeQuality.validity : dms::SourceValidity::Missing;
                qualityHeader.confidence = semanticSuccess ? eyeQuality.confidence : 0.0F;
                qualityHeader.visibility = semanticSuccess ? eyeQuality.visibility : 0.0F;
                if (calibratedPose && (std::abs(calibratedPose->yawDegrees) > 35.0F ||
                                       std::abs(calibratedPose->pitchDegrees) > 25.0F))
                    qualityHeader.confidence = 0.0F;
                eyeInput.usability = qualityGate.update(qualityHeader, input.timestamp());
                if (semanticSuccess)
                {
                    eyeInput.rightEar = static_cast<float>(tracker.getRightEAR());
                    eyeInput.leftEar = static_cast<float>(tracker.getLeftEAR());
                }
                if (!eyeCalibrationApplied)
                {
                    const auto calibratedEyes = eyeCalibrator.update(input.timestamp(), eyeInput.usability,
                                                                     eyeInput.rightEar, eyeInput.leftEar);
                    if (calibratedEyes)
                    {
                        eyeMetrics.setCalibration(*calibratedEyes);
                        eyeCalibrationApplied = true;
                        calibrationCompletedOnce = true;
                    }
                    else
                        eyeInput.usability = dms::ObservationUsability::Recovering;
                }
                finalEyeMetrics = eyeMetrics.update(eyeInput);
                if (finalEyeMetrics.state == dms::EyeState::Unknown)
                    ++unknownEyeFrames;
                else
                    ++usableEyeFrames;
                if (finalEyeMetrics.prolongedClosure)
                    ++prolongedClosureFrames;
                if (qualityHeader.validity == dms::SourceValidity::Occluded)
                    ++occludedEyeFrames;
                else if (semanticSuccess && eyeInput.usability != dms::ObservationUsability::Usable &&
                         eyeInput.usability != dms::ObservationUsability::Recovering)
                    ++lowQualityEyeFrames;

                const auto qualityCalibrationEnd = Clock::now();
                const auto temporalFsmsStart = qualityCalibrationEnd;

                const auto geometryUsability = faceGeometryValid
                    ? dms::ObservationUsability::Usable : dms::ObservationUsability::Missing;
                finalYawn = yawnFsm.update(input.timestamp(), geometryUsability, mouthOpenness);
                const float maximumPoseError = std::max(
                    5.0F, static_cast<float>(faceResult.faceBox.width) * 0.15F);
                finalHeadPose = headPoseFsm.update(
                    input.timestamp(), calibratedPose && calibratedPose->reprojectionErrorPixels <= maximumPoseError
                        ? dms::ObservationUsability::Usable : dms::ObservationUsability::LowConfidence,
                    calibratedPose ? std::optional<float>(calibratedPose->yawDegrees) : std::nullopt,
                    calibratedPose ? std::optional<float>(calibratedPose->pitchDegrees) : std::nullopt);
                const auto gazeUsability = gazeValid && gaze.interEyeAgreement >= 0.30F &&
                                                   eyeInput.usability == dms::ObservationUsability::Usable &&
                                                   finalEyeMetrics.state == dms::EyeState::Open
                    ? dms::ObservationUsability::Usable : dms::ObservationUsability::Missing;
                finalDistraction = distractionFsm.update(
                    input.timestamp(), gazeUsability,
                    gazeValid ? std::optional<float>(gaze.horizontal) : std::nullopt,
                    gazeValid ? std::optional<float>(gaze.vertical) : std::nullopt,
                    finalHeadPose.zone);
                finalPresence = presenceFsm.update(
                    input.timestamp(), faceResult.detected ? dms::ObservationUsability::Usable
                                                           : dms::ObservationUsability::Missing,
                    faceResult.detected);
                finalDrowsiness = drowsinessFsm.update(
                    {input.timestamp(), eyeInput.usability, finalPresence,
                     finalEyeMetrics.perclos, finalEyeMetrics.prolongedClosure,
                     finalEyeMetrics.longBlinkEvent, finalYawn.event});
                finalAvailability = availabilityFsm.update(input.timestamp(), eyeInput.usability);
                finalEyeUsability = eyeInput.usability;
                finalGazeAvailable = gazeValid;
                if (finalAvailability.unavailable) ++monitoringUnavailableFrames;
                if (finalAvailability.notify) ++monitoringNotifyFrames;
                if (finalDistraction.distracted)
                    ++distractedFrames;
                if (finalDistraction.event)
                    ++distractionEventCount;
                const auto temporalFsmsEnd = Clock::now();

                samples.capture.push_back(milliseconds(captureEnd - captureStart));
                samples.backend.push_back(milliseconds(backendEnd - backendStart));
                samples.semantic.push_back(milliseconds(semanticEnd - semanticStart));
                samples.total.push_back(milliseconds(temporalFsmsEnd - totalStart));
                samples.faceGeometry.push_back(milliseconds(faceGeometryEnd - faceGeometryStart));
                samples.eyeMappingAndEar.push_back(milliseconds(eyeMappingEnd - eyeMappingStart));
                samples.eyeQualityAndCalibration.push_back(
                    milliseconds(qualityCalibrationEnd - qualityCalibrationStart));
                samples.temporalFsms.push_back(milliseconds(temporalFsmsEnd - temporalFsmsStart));
                const auto outputStart = Clock::now();
                if (backendSuccess)
                    ++successfulFrames;
                if (backendSuccess && faceResult.detected)
                    ++detectedFrames;
                ++measuredFrames;
                if (trace)
                {
                    trace << (index - options.warmupFrames) << ','
                          << std::chrono::duration<double, std::milli>(input.timestamp()).count() << ','
                          << (backendSuccess ? 1 : 0) << ',' << (faceResult.detected ? 1 : 0) << ','
                          << (faceResult.landmarksValid ? 1 : 0) << ',' << (semanticSuccess ? 1 : 0) << ',';
                    if (semanticSuccess)
                    {
                        trace << tracker.getRightEAR() << ',' << tracker.getLeftEAR() << ',' << tracker.getAverageEAR()
                              << ',' << (tracker.isEyeClosed() ? 1 : 0) << ',' << tracker.getBlinkCount();
                    }
                    else
                        trace << ",,,,,";
                    trace << ',' << usabilityName(eyeInput.usability) << ',';
                    if (finalEyeMetrics.openness)
                        trace << *finalEyeMetrics.openness;
                    trace << ',' << eyeStateName(finalEyeMetrics.state) << ',' << (finalEyeMetrics.blinkEvent ? 1 : 0)
                          << ',' << finalEyeMetrics.blinkCount << ','
                          << (finalEyeMetrics.longBlinkEvent ? 1 : 0) << ',' << finalEyeMetrics.longBlinkCount << ','
                          << std::chrono::duration<double, std::milli>(finalEyeMetrics.closureDuration).count() << ','
                          << (finalEyeMetrics.prolongedClosure ? 1 : 0) << ','
                          << (finalEyeMetrics.prolongedClosureEvent ? 1 : 0) << ','
                          << finalEyeMetrics.prolongedClosureCount << ',';
                    if (finalEyeMetrics.perclos)
                        trace << *finalEyeMetrics.perclos;
                    trace << ',' << finalEyeMetrics.perclosCoverage << ','
                          << eyeQuality.confidence << ',' << eyeQuality.visibility << ','
                          << eyeQuality.meanIntensity << ',' << eyeQuality.contrast << ','
                          << eyeQuality.laplacianVariance << ',';
                    if (mouthOpenness) trace << *mouthOpenness;
                    trace << ',' << (finalYawn.active ? 1 : 0) << ','
                          << (finalYawn.event ? 1 : 0) << ',' << finalYawn.count << ',';
                    if (calibratedPose) trace << calibratedPose->yawDegrees;
                    trace << ',';
                    if (calibratedPose) trace << calibratedPose->pitchDegrees;
                    trace << ',';
                    if (calibratedPose) trace << calibratedPose->reprojectionErrorPixels;
                    trace << ',' << headZoneName(finalHeadPose.zone) << ','
                          << (finalHeadPose.movementEvent ? 1 : 0) << ','
                          << finalHeadPose.leftCount << ',' << finalHeadPose.rightCount << ','
                          << finalHeadPose.upCount << ',' << finalHeadPose.downCount << ',';
                    if (gazeValid) trace << gaze.horizontal;
                    trace << ',';
                    if (gazeValid) trace << gaze.vertical;
                    trace << ',';
                    if (gazeValid) trace << gaze.interEyeAgreement;
                    trace << ',' << gazeZoneName(finalDistraction.gaze) << ','
                          << (finalDistraction.distracted ? 1 : 0) << ','
                          << (finalDistraction.event ? 1 : 0) << ','
                          << presenceName(finalPresence) << ','
                          << (finalAvailability.unavailable ? 1 : 0) << ',' << (finalAvailability.notify ? 1 : 0) << ','
                          << finalAvailability.episodeCount << ','
                          << drowsinessName(finalDrowsiness.state) << ','
                          << finalDrowsiness.recentYawns;
                    trace << ',' << faceResult.faceBox.x << ',' << faceResult.faceBox.y << ','
                          << faceResult.faceBox.width << ',' << faceResult.faceBox.height << ','
                          << milliseconds(backendEnd - backendStart) << ','
                          << milliseconds(temporalFsmsEnd - totalStart)
                          << '\n';
                }
                if (options.sponsorDemo)
                {
                    if (faceResult.detected)
                        cv::rectangle(frame, faceResult.faceBox, cv::Scalar(255, 120, 0), 2);
                    cv::Mat displayFrame;
                    if (frame.cols < 960)
                        cv::resize(frame, displayFrame, {}, 2.0, 2.0, cv::INTER_LINEAR);
                    else
                        displayFrame = frame.clone();
                    const double frameMs = milliseconds(semanticEnd - totalStart);
                    const double processingFps = frameMs > 0.0 ? 1000.0 / frameMs : 0.0;
                    finalDisplayFrame = displayFrame.clone();
                    drawSponsorOverlay(displayFrame, options.backend, input.timestamp(), finalEyeUsability,
                                       finalEyeMetrics, finalYawn, finalHeadPose, finalDistraction,
                                       finalPresence, finalAvailability, finalDrowsiness, processingFps,
                                       finalGazeAvailable, distractionEventCount, false);
                    cv::imshow("Face DMS Sponsor Engineering Demo", displayFrame);
                    ++renderedFrames;
                    const int frameDelay = static_cast<int>(std::lround(1000.0 / input.framesPerSecond()));
                    const int wait = std::max(1, frameDelay - static_cast<int>(std::lround(frameMs)));
                    const int key = cv::waitKey(wait);
                    if (key == 27 || key == 'q' || key == 'Q')
                    {
                        userStoppedDemo = true;
                        samples.output.push_back(milliseconds(Clock::now() - outputStart));
                        break;
                    }
                }
                samples.output.push_back(milliseconds(Clock::now() - outputStart));
            }
        }
        const auto wallEnd = Clock::now();
        if (options.resourceProfile)
        {
            resourceProfiler.setPhase("shutdown");
            resourceProfiler.stop();
        }
        if (!options.resourceTrace.empty() &&
            !resourceProfiler.writeCsv(options.resourceTrace, error))
        {
            std::cerr << "Error: " << error << '\n';
            return 1;
        }
        const std::clock_t cpuEnd = std::clock();
        const double wallSeconds = std::chrono::duration<double>(wallEnd - wallStart).count();
        const double measuredSeconds = [&]() {
            double sum = 0.0;
            for (double value : samples.total)
                sum += value;
            return sum / 1000.0;
        }();
        const unsigned int logicalCpus = std::max(1U, std::thread::hardware_concurrency());
        const double cpuSeconds = static_cast<double>(cpuEnd - cpuStart) / CLOCKS_PER_SEC;
        const double cpuPercentCapacity = wallSeconds > 0.0 ? 100.0 * cpuSeconds / wallSeconds / logicalCpus : 0.0;
        const std::uint64_t finalResidentMemory = currentResidentMemoryBytes();
        const std::int64_t residentMemoryGrowth =
            static_cast<std::int64_t>(finalResidentMemory) - static_cast<std::int64_t>(initialResidentMemory);

        std::ostringstream json;
        json << std::fixed << std::setprecision(6) << "{\n"
             << "  \"schema_version\": 6,\n"
             << "  \"backend\": \"" << backendName(options.backend) << "\",\n"
             << "  \"build_configuration\": \"" << buildConfiguration() << "\",\n"
             << "  \"benchmark_metadata\": {\"source_revision\": \"" << FACE_SOURCE_REVISION
             << "\", \"platform\": \"" << platformName() << "\", \"compiler\": \""
             << compilerName() << "\", \"resource_sample_interval_ms\": "
             << options.resourceSampleMilliseconds << ", \"resource_profiling_enabled\": "
             << (options.resourceProfile ? "true" : "false") << "},\n"
             << "  \"input\": \"" << jsonEscape(options.input.string()) << "\",\n"
             << "  \"input_kind\": \"" << input.kind() << "\",\n"
             << "  \"input_width\": " << inputSize.width << ",\n"
             << "  \"input_height\": " << inputSize.height << ",\n"
             << "  \"warmup_frames\": " << options.warmupFrames << ",\n"
             << "  \"measured_frames\": " << measuredFrames << ",\n"
             << "  \"successful_frames\": " << successfulFrames << ",\n"
             << "  \"detected_frames\": " << detectedFrames << ",\n"
             << "  \"dropped_frames\": 0,\n"
             << "  \"superseded_frames\": 0,\n"
             << "  \"rendered_frames\": " << renderedFrames << ",\n"
             << "  \"eye_crop_pairs_written\": " << eyeCropPairsWritten << ",\n"
             << "  \"eye_crop_failures\": " << eyeCropFailures << ",\n"
             << "  \"throughput_fps\": " << (measuredSeconds > 0.0 ? measuredFrames / measuredSeconds : 0.0)
             << ",\n"
             << "  \"process_cpu_percent_of_total_capacity\": " << cpuPercentCapacity << ",\n"
             << "  \"logical_cpu_count\": " << logicalCpus << ",\n"
             << "  \"initial_resident_memory_bytes\": " << initialResidentMemory << ",\n"
             << "  \"final_resident_memory_bytes\": " << finalResidentMemory << ",\n"
             << "  \"resident_memory_growth_bytes\": " << residentMemoryGrowth << ",\n"
             << "  \"peak_resident_memory_bytes\": " << peakResidentMemoryBytes() << ",\n"
             << "  \"resource_profile\": {\n";
        writeResourceSummary(json, "overall", resourceProfiler.summarize(), true);
        writeResourceSummary(json, "startup", resourceProfiler.summarize("startup"), true);
        writeResourceSummary(json, "warmup", resourceProfiler.summarize("warmup"), true);
        writeResourceSummary(json, "calibration", resourceProfiler.summarize("calibration"), true);
        writeResourceSummary(json, "processing", resourceProfiler.summarize("processing"), true);
        writeResourceSummary(json, "recalibration", resourceProfiler.summarize("recalibration"), false);
        json << "  },\n"
             << "  \"eye_metrics\": {\n"
             << "    \"policy_profile\": \"" << policy.name << "\",\n"
             << "    \"closed_ear_calibration\": " << eyeMetrics.calibration().closedEar << ",\n"
             << "    \"open_ear_calibration\": " << eyeMetrics.calibration().openEar << ",\n"
             << "    \"usable_frames\": " << usableEyeFrames << ",\n"
             << "    \"unknown_frames\": " << unknownEyeFrames << ",\n"
             << "    \"blink_count\": " << finalEyeMetrics.blinkCount << ",\n"
             << "    \"long_blink_count\": " << finalEyeMetrics.longBlinkCount << ",\n"
             << "    \"prolonged_closure_count\": " << finalEyeMetrics.prolongedClosureCount << ",\n"
             << "    \"prolonged_closure_frames\": " << prolongedClosureFrames << ",\n"
             << "    \"final_perclos\": ";
        if (finalEyeMetrics.perclos)
            json << *finalEyeMetrics.perclos;
        else
            json << "null";
        json << ",\n"
             << "    \"final_perclos_coverage\": " << finalEyeMetrics.perclosCoverage << ",\n"
             << "    \"low_quality_frames\": " << lowQualityEyeFrames << ",\n"
             << "    \"occluded_frames\": " << occludedEyeFrames << "\n"
             << "  },\n"
             << "  \"temporal_events\": {\n"
             << "    \"yawn_count\": " << finalYawn.count << ",\n"
             << "    \"head_left_count\": " << finalHeadPose.leftCount << ",\n"
             << "    \"head_right_count\": " << finalHeadPose.rightCount << ",\n"
             << "    \"head_up_count\": " << finalHeadPose.upCount << ",\n"
             << "    \"head_down_count\": " << finalHeadPose.downCount << ",\n"
             << "    \"distracted_frames\": " << distractedFrames << ",\n"
             << "    \"distraction_event_count\": " << distractionEventCount << ",\n"
             << "    \"monitoring_unavailable_frames\": " << monitoringUnavailableFrames << ",\n"
             << "    \"monitoring_notify_frames\": " << monitoringNotifyFrames << ",\n"
             << "    \"monitoring_episode_count\": " << finalAvailability.episodeCount << ",\n"
             << "    \"final_presence\": \"" << presenceName(finalPresence) << "\",\n"
             << "    \"final_drowsiness\": \"" << drowsinessName(finalDrowsiness.state) << "\"\n"
             << "  },\n"
             << "  \"latency_ms\": {\n";
        writeSummary(json, "capture", summarize(samples.capture), true);
        writeSummary(json, "backend", summarize(samples.backend), true);
        writeSummary(json, "semantic", summarize(samples.semantic), true);
        writeSummary(json, "face_geometry", summarize(samples.faceGeometry), true);
        writeSummary(json, "eye_mapping_and_ear", summarize(samples.eyeMappingAndEar), true);
        writeSummary(json, "eye_quality_and_calibration", summarize(samples.eyeQualityAndCalibration), true);
        writeSummary(json, "temporal_fsms", summarize(samples.temporalFsms), true);
        writeSummary(json, "output", summarize(samples.output), true);
        writeSummary(json, "end_to_end", summarize(samples.total), false);
        json << "  }\n}\n";

        if (!options.output.empty())
        {
            std::ofstream file(options.output, std::ios::binary | std::ios::trunc);
            if (!file)
            {
                std::cerr << "Error: Cannot write benchmark output: " << options.output << '\n';
                return 1;
            }
            file << json.str();
        }
        std::cout << json.str();
        if (options.sponsorDemo && !options.sponsorDemoAutoExit &&
            !userStoppedDemo && !finalDisplayFrame.empty())
        {
            drawSponsorOverlay(finalDisplayFrame, options.backend, input.timestamp(), finalEyeUsability,
                               finalEyeMetrics, finalYawn, finalHeadPose, finalDistraction,
                               finalPresence, finalAvailability, finalDrowsiness,
                               measuredSeconds > 0.0 ? measuredFrames / measuredSeconds : 0.0,
                               finalGazeAvailable, distractionEventCount, true);
            cv::putText(finalDisplayFrame, "Press any key to exit", {25, finalDisplayFrame.rows - 25},
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
            cv::imshow("Face DMS Sponsor Engineering Demo", finalDisplayFrame);
            cv::waitKey(0);
        }
        if (options.sponsorDemo) cv::destroyAllWindows();
        return successfulFrames == measuredFrames ? 0 : 1;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Fatal benchmark error: " << exception.what() << '\n';
        return 1;
    }
}
