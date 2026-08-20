#include "../api/FaceMediaPipe.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

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
        mediapipe::tasks::vision::core::RunningMode::IMAGE;
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
        bgr == nullptr || result == nullptr || width <= 0 || height <= 0 ||
        stride < width * 3 || result->landmarks == nullptr ||
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
    // types across the DLL ABI.
    mediapipe::ImageFrame rgb_frame(
        mediapipe::ImageFormat::SRGB,
        width,
        height,
        stride);

    uint8_t* rgb = rgb_frame.MutablePixelData();
    const int rgb_stride = rgb_frame.WidthStep();

    for (int y = 0; y < height; ++y)
    {
        const uint8_t* src = bgr + static_cast<size_t>(y) * stride;
        uint8_t* dst = rgb + static_cast<size_t>(y) * rgb_stride;

        for (int x = 0; x < width; ++x)
        {
            dst[3 * x + 0] = src[3 * x + 2];
            dst[3 * x + 1] = src[3 * x + 1];
            dst[3 * x + 2] = src[3 * x + 0];
        }
    }

    mediapipe::Image image(
        std::make_shared<mediapipe::ImageFrame>(std::move(rgb_frame)));

    auto detection_or = handle->landmarker->Detect(std::move(image));

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
    const int count = std::min<int>(
        static_cast<int>(landmarks.size()),
        result->landmark_capacity);

    float min_x = static_cast<float>(width);
    float min_y = static_cast<float>(height);
    float max_x = 0.0f;
    float max_y = 0.0f;

    for (int i = 0; i < count; ++i)
    {
        const auto& landmark = landmarks[i];

        const float x =
            landmark.x * static_cast<float>(width);
        const float y =
            landmark.y * static_cast<float>(height);

        result->landmarks[i].x = x;
        result->landmarks[i].y = y;
        result->landmarks[i].z = landmark.z;

        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }

    result->landmark_count = count;
    result->detected = count > 0 ? 1 : 0;

    if (result->detected)
    {
        result->face_x = static_cast<int32_t>(std::floor(min_x));
        result->face_y = static_cast<int32_t>(std::floor(min_y));
        result->face_width = static_cast<int32_t>(
            std::ceil(max_x) - std::floor(min_x));
        result->face_height = static_cast<int32_t>(
            std::ceil(max_y) - std::floor(min_y));
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
