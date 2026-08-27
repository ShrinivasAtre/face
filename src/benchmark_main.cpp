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
#include "FaceBackend.hpp"
#include "PfldEyeLandmarkMapper.hpp"
#include "RecordedFrameClock.hpp"
#include "YuNetLbfBackend.hpp"
#include "YuNetPfldBackend.hpp"

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
        if (!capture_.isOpened())
            capture_.open(path.string());
        if (!capture_.isOpened())
            return false;
        kind_ = "video";
        clock_.reset(capture_.get(cv::CAP_PROP_FPS));
        return true;
    }

    bool next(cv::Mat &frame)
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
        InputFrames input;
        if (!input.open(options.input))
        {
            std::cerr << "Error: Cannot open benchmark input: " << options.input << '\n';
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
        const dms::EyeCalibration eyeCalibration;
        const dms::EyeTemporalConfig eyeConfig;
        dms::EyeTemporalMetrics eyeMetrics(eyeCalibration, eyeConfig);
        dms::ObservationQualityGateConfig qualityConfig;
        qualityConfig.policy = {0.5F, 0.5F, std::chrono::milliseconds(250)};
        qualityConfig.reacquisitionConfirmation = std::chrono::milliseconds(100);
        dms::ObservationQualityGate qualityGate(qualityConfig);
        EyeQualityAssessor eyeQualityAssessor;
        dms::YawnFsm yawnFsm({});
        dms::HeadPoseFsm headPoseFsm({});
        HeadPoseNeutralCalibrator headPoseCalibrator;
        dms::DistractionFsm distractionFsm({});
        dms::DriverPresenceFsm presenceFsm({});
        dms::DrowsinessFsm drowsinessFsm({});
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
                     "temporal_blink_event,temporal_blink_count,closure_ms,"
                     "prolonged_closure,perclos,perclos_coverage,"
                     "eye_quality_confidence,eye_visibility,eye_mean,eye_contrast,eye_laplacian,"
                     "mouth_openness,yawn_active,yawn_event,yawn_count,"
                     "yaw_degrees,pitch_degrees,pose_reprojection,head_zone,head_event,"
                     "left_count,right_count,up_count,down_count,"
                     "gaze_horizontal,gaze_vertical,gaze_agreement,gaze_zone,"
                     "distracted,distraction_event,presence,drowsiness,recent_yawns,"
                     "face_x,face_y,face_width,face_height,"
                     "backend_ms,end_to_end_ms\n";
            trace << std::fixed << std::setprecision(6);
        }
        cv::Mat frame;
        FaceResult faceResult;
        Samples samples;
        samples.capture.reserve(options.measuredFrames);
        samples.backend.reserve(options.measuredFrames);
        samples.semantic.reserve(options.measuredFrames);
        samples.total.reserve(options.measuredFrames);
        std::size_t detectedFrames = 0;
        std::size_t successfulFrames = 0;
        std::size_t usableEyeFrames = 0;
        std::size_t unknownEyeFrames = 0;
        std::size_t prolongedClosureFrames = 0;
        std::size_t lowQualityEyeFrames = 0;
        std::size_t occludedEyeFrames = 0;
        std::size_t distractedFrames = 0;
        dms::EyeMetricResult finalEyeMetrics;
        dms::YawnResult finalYawn;
        dms::HeadPoseResult finalHeadPose;
        dms::DistractionResult finalDistraction;
        dms::PresenceState finalPresence = dms::PresenceState::Unknown;
        dms::DrowsinessResult finalDrowsiness;
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
            SemanticEyeLandmarks eyes;
            bool eyeMapped = false;
            SemanticFaceGeometry faceGeometry;
            const bool faceGeometryValid = mapBackendFaceGeometry(options.backend, faceResult, faceGeometry);
            const std::optional<float> mouthOpenness = faceGeometryValid
                ? calculateMouthOpenness(faceGeometry) : std::nullopt;
            HeadPoseAngles poseAngles;
            const bool poseValid = faceGeometryValid &&
                estimateHeadPose(faceGeometry, frame.size(), poseAngles);
            SemanticGaze gaze;
            const bool gazeValid = mapBackendGaze(options.backend, faceResult, gaze);
            if (backendSuccess && faceResult.detected)
            {
                eyeMapped = options.backend == BackendKind::Pfld
                                ? mapPfldEyeLandmarks(faceResult.landmarks, eyes)
                                : mapBackendEyeLandmarks(options.backend, faceResult, eyes);
                if (eyeMapped)
                {
                    semanticSuccess = tracker.process(frame, eyes);
                }
            }
            const auto semanticEnd = Clock::now();

            if (index >= options.warmupFrames)
            {
                dms::EyeMetricInput eyeInput;
                eyeInput.timestamp = input.timestamp();
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
                                                   eyeInput.usability == dms::ObservationUsability::Usable
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
                     finalYawn.event});
                if (finalDistraction.distracted)
                    ++distractedFrames;

                samples.capture.push_back(milliseconds(captureEnd - captureStart));
                samples.backend.push_back(milliseconds(backendEnd - backendStart));
                samples.semantic.push_back(milliseconds(semanticEnd - semanticStart));
                samples.total.push_back(milliseconds(semanticEnd - totalStart));
                if (backendSuccess)
                    ++successfulFrames;
                if (backendSuccess && faceResult.detected)
                    ++detectedFrames;
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
                          << std::chrono::duration<double, std::milli>(finalEyeMetrics.closureDuration).count() << ','
                          << (finalEyeMetrics.prolongedClosure ? 1 : 0) << ',';
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
                          << drowsinessName(finalDrowsiness.state) << ','
                          << finalDrowsiness.recentYawns;
                    trace << ',' << faceResult.faceBox.x << ',' << faceResult.faceBox.y << ','
                          << faceResult.faceBox.width << ',' << faceResult.faceBox.height << ','
                          << milliseconds(backendEnd - backendStart) << ',' << milliseconds(semanticEnd - totalStart)
                          << '\n';
                }
            }
        }
        const auto wallEnd = Clock::now();
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
             << "  \"schema_version\": 4,\n"
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
             << "  \"throughput_fps\": " << (measuredSeconds > 0.0 ? options.measuredFrames / measuredSeconds : 0.0)
             << ",\n"
             << "  \"process_cpu_percent_of_total_capacity\": " << cpuPercentCapacity << ",\n"
             << "  \"logical_cpu_count\": " << logicalCpus << ",\n"
             << "  \"initial_resident_memory_bytes\": " << initialResidentMemory << ",\n"
             << "  \"final_resident_memory_bytes\": " << finalResidentMemory << ",\n"
             << "  \"resident_memory_growth_bytes\": " << residentMemoryGrowth << ",\n"
             << "  \"peak_resident_memory_bytes\": " << peakResidentMemoryBytes() << ",\n"
             << "  \"eye_metrics\": {\n"
             << "    \"closed_ear_calibration\": " << eyeCalibration.closedEar << ",\n"
             << "    \"open_ear_calibration\": " << eyeCalibration.openEar << ",\n"
             << "    \"usable_frames\": " << usableEyeFrames << ",\n"
             << "    \"unknown_frames\": " << unknownEyeFrames << ",\n"
             << "    \"blink_count\": " << finalEyeMetrics.blinkCount << ",\n"
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
             << "    \"final_presence\": \"" << presenceName(finalPresence) << "\",\n"
             << "    \"final_drowsiness\": \"" << drowsinessName(finalDrowsiness.state) << "\"\n"
             << "  },\n"
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
                std::cerr << "Error: Cannot write benchmark output: " << options.output << '\n';
                return 1;
            }
            file << json.str();
        }
        std::cout << json.str();
        return successfulFrames == options.measuredFrames ? 0 : 1;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Fatal benchmark error: " << exception.what() << '\n';
        return 1;
    }
}
