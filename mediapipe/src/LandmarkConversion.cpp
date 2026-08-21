#include "LandmarkConversion.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace face_mp_internal {
namespace {

void reset_result(FaceMPResult* result)
{
    result->detected = 0;
    result->face_x = 0;
    result->face_y = 0;
    result->face_width = 0;
    result->face_height = 0;
    result->landmark_count = 0;
}

}  // namespace

bool write_face_result(
    const NormalizedLandmark* source,
    int32_t source_count,
    int32_t image_width,
    int32_t image_height,
    FaceMPResult* result)
{
    if (result == nullptr || source_count < 0 ||
        image_width <= 0 || image_height <= 0)
    {
        return false;
    }

    reset_result(result);

    if (source_count == 0)
    {
        return true;
    }

    if (source == nullptr || result->landmarks == nullptr ||
        result->landmark_capacity <= 0)
    {
        return false;
    }

    float min_x = static_cast<float>(image_width);
    float min_y = static_cast<float>(image_height);
    float max_x = 0.0f;
    float max_y = 0.0f;

    const int32_t written =
        std::min(source_count, result->landmark_capacity);

    for (int32_t i = 0; i < source_count; ++i)
    {
        const NormalizedLandmark& landmark = source[i];

        if (!std::isfinite(landmark.x) ||
            !std::isfinite(landmark.y) ||
            !std::isfinite(landmark.z))
        {
            reset_result(result);
            return false;
        }

        const float pixel_x =
            landmark.x * static_cast<float>(image_width);
        const float pixel_y =
            landmark.y * static_cast<float>(image_height);

        if (i < written)
        {
            result->landmarks[i].x = pixel_x;
            result->landmarks[i].y = pixel_y;
            result->landmarks[i].z = landmark.z;
        }

        const float bounded_x = std::clamp(
            pixel_x, 0.0f, static_cast<float>(image_width));
        const float bounded_y = std::clamp(
            pixel_y, 0.0f, static_cast<float>(image_height));

        min_x = std::min(min_x, bounded_x);
        min_y = std::min(min_y, bounded_y);
        max_x = std::max(max_x, bounded_x);
        max_y = std::max(max_y, bounded_y);
    }

    const int32_t left = static_cast<int32_t>(std::floor(min_x));
    const int32_t top = static_cast<int32_t>(std::floor(min_y));
    const int32_t right = static_cast<int32_t>(std::ceil(max_x));
    const int32_t bottom = static_cast<int32_t>(std::ceil(max_y));

    result->detected = 1;
    result->landmark_count = written;
    result->face_x = left;
    result->face_y = top;
    result->face_width = std::max<int32_t>(0, right - left);
    result->face_height = std::max<int32_t>(0, bottom - top);
    return true;
}

}  // namespace face_mp_internal
