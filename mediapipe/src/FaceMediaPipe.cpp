#include "../api/FaceMediaPipe.h"
#include "BgrToRgb.h"
#include "LandmarkConversion.h"
#include "MonotonicTimestamp.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/tasks/cc/vision/face_landmarker/face_landmarker.h"

namespace {

constexpr uint32_t kFaceMediaPipeApiVersion = 1;

using FaceLandmarker =
    mediapipe::tasks::vision::face_landmarker::FaceLandmarker;
using FaceLandmarkerOptions =
    mediapipe::tasks::vision::face_landmarker::FaceLandmarkerOptions;
using FaceLandmarkerResult =
    mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult;

}  // namespace

struct FaceMPHandle
{
    std::unique_ptr<FaceLandmarker> landmarker;
    std::string last_error;
    face_mp_internal::MonotonicTimestamp timestamp;
    std::vector<face_mp_internal::NormalizedLandmark> normalized_landmarks;
};

extern "C" FACE_MEDIAPIPE_API uint32_t face_mp_api_version()
{
    return kFaceMediaPipeApiVersion;
}

extern "C" FACE_MEDIAPIPE_API FaceMPHandle* face_mp_create(
    const char* model_path)
{
    if (model_path == nullptr || model_path[0] == '\0')
    {
        return nullptr;
    }

    auto handle = std::make_unique<FaceMPHandle>();

    auto options = std::make_unique<FaceLandmarkerOptions>();
    options->base_options.model_asset_path = model_path;
    options->running_mode =
        mediapipe::tasks::vision::core::RunningMode::VIDEO;
    options->num_faces = 1;
    options->min_face_detection_confidence = 0.5f;
    options->min_face_presence_confidence = 0.5f;
    options->min_tracking_confidence = 0.5f;
    options->output_face_blendshapes = false;
    options->output_facial_transformation_matrixes = false;

    auto landmarker_or = FaceLandmarker::Create(std::move(options));

    if (!landmarker_or.ok())
    {
        return nullptr;
    }

    handle->landmarker = std::move(landmarker_or.value());
    return handle.release();
}

extern "C" FACE_MEDIAPIPE_API int32_t face_mp_process_bgr(
    FaceMPHandle* handle,
    const uint8_t* bgr,
    int32_t width,
    int32_t height,
    int32_t stride,
    FaceMPResult* result)
{
    if (handle == nullptr || handle->landmarker == nullptr ||
        result == nullptr ||
        !face_mp_internal::is_valid_bgr_image(
            bgr, width, height, stride) ||
        result->landmarks == nullptr ||
        result->landmark_capacity <= 0)
    {
        return 0;
    }

    result->detected = 0;
    result->face_x = 0;
    result->face_y = 0;
    result->face_width = 0;
    result->face_height = 0;
    result->landmark_count = 0;

    // MediaPipe Face Landmarker expects RGB or RGBA. Convert the caller's
    // OpenCV-style BGR buffer to an RGB ImageFrame without exposing OpenCV
    // types across the DLL ABI. The 4-argument ImageFrame constructor takes
    // an alignment boundary, not a source stride, so use MediaPipe's default
    // allocation and honor its actual WidthStep() while copying each row.
    mediapipe::ImageFrame rgb_frame(
        mediapipe::ImageFormat::SRGB,
        width,
        height);

    uint8_t* rgb = rgb_frame.MutablePixelData();
    const int rgb_stride = rgb_frame.WidthStep();

    if (!face_mp_internal::copy_bgr_to_rgb(
            bgr,
            width,
            height,
            stride,
            rgb,
            rgb_stride))
    {
        handle->last_error = "BGR to RGB conversion failed";
        return 0;
    }

    mediapipe::Image image(
        std::make_shared<mediapipe::ImageFrame>(std::move(rgb_frame)));

    const int64_t timestamp_ms = handle->timestamp.next(
        face_mp_internal::MonotonicTimestamp::Clock::now());
    auto detection_or = handle->landmarker->DetectForVideo(
        std::move(image), timestamp_ms);

    if (!detection_or.ok())
    {
        handle->last_error = detection_or.status().message();
        return 0;
    }

    const FaceLandmarkerResult& detection = detection_or.value();

    if (detection.face_landmarks.empty())
    {
        handle->last_error.clear();
        return 1;
    }

    const auto& landmarks = detection.face_landmarks.front().landmarks;

    auto& normalized = handle->normalized_landmarks;
    normalized.clear();
    normalized.reserve(landmarks.size());

    for (const auto& landmark : landmarks)
    {
        normalized.push_back({
            landmark.x,
            landmark.y,
            landmark.z,
        });
    }

    if (!face_mp_internal::write_face_result(
            normalized.data(),
            static_cast<int32_t>(normalized.size()),
            width,
            height,
            result))
    {
        handle->last_error = "Landmark conversion failed";
        return 0;
    }

    handle->last_error.clear();
    return 1;
}

extern "C" FACE_MEDIAPIPE_API const char* face_mp_last_error(
    FaceMPHandle* handle)
{
    if (handle == nullptr)
    {
        return "invalid FaceMPHandle";
    }

    return handle->last_error.c_str();
}

extern "C" FACE_MEDIAPIPE_API void face_mp_destroy(
    FaceMPHandle* handle)
{
    delete handle;
}
