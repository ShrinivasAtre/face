#include "MediaPipeBackend.hpp"

#include <opencv2/core.hpp>
#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

MediaPipeBackend::MediaPipeBackend(std::filesystem::path libraryPath)
    : libraryPath_(std::move(libraryPath)) {}

MediaPipeBackend::~MediaPipeBackend()
{
    if (handle_ != nullptr)
    {
        runtime_.destroy(handle_);
        handle_ = nullptr;
    }
}

bool MediaPipeBackend::initialize(const std::string& modelPath)
{
    if (handle_ != nullptr)
    {
        runtime_.destroy(handle_);
        handle_ = nullptr;
    }
    runtime_.unload();
    if (!runtime_.load(libraryPath_))
    {
        std::cerr << "ERROR: " << runtime_.diagnostic() << '\n';
        return false;
    }
    handle_ = runtime_.create(modelPath.c_str());
    if (handle_ == nullptr)
    {
        std::cerr << "ERROR: FaceMediaPipe could not create a landmarker for: "
                  << modelPath << '\n';
        runtime_.unload();
        return false;
    }
    return true;
}

bool MediaPipeBackend::process(const cv::Mat& frame, FaceResult& result)
{
    result.detected = false;
    result.landmarksValid = false;
    result.faceBox = {};
    result.landmarks.clear();
    if (handle_ == nullptr || frame.empty() || frame.type() != CV_8UC3 ||
        frame.cols > std::numeric_limits<std::int32_t>::max() ||
        frame.rows > std::numeric_limits<std::int32_t>::max() ||
        frame.step > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
    {
        return false;
    }

    FaceMPResult bridgeResult{};
    bridgeResult.landmarks = bridgeLandmarks_.data();
    bridgeResult.landmark_capacity =
        static_cast<std::int32_t>(bridgeLandmarks_.size());
    if (runtime_.processBgr(handle_, frame.data, frame.cols, frame.rows,
                            static_cast<std::int32_t>(frame.step),
                            &bridgeResult) != 1)
    {
        std::cerr << "ERROR: FaceMediaPipe processing failed: "
                  << runtime_.lastError(handle_) << '\n';
        return false;
    }

    if (bridgeResult.landmark_count < 0 ||
        bridgeResult.landmark_count > bridgeResult.landmark_capacity)
    {
        return false;
    }
    result.detected = bridgeResult.detected != 0;
    result.faceBox = cv::Rect(bridgeResult.face_x, bridgeResult.face_y,
                              bridgeResult.face_width, bridgeResult.face_height);
    result.landmarks.reserve(static_cast<std::size_t>(bridgeResult.landmark_count));
    for (std::int32_t index = 0; index < bridgeResult.landmark_count; ++index)
    {
        const auto& landmark = bridgeLandmarks_[static_cast<std::size_t>(index)];
        result.landmarks.push_back({landmark.x, landmark.y, landmark.z});
    }
    result.landmarksValid = result.detected && !result.landmarks.empty();
    return true;
}

const char* MediaPipeBackend::name() const { return "mediapipe"; }
