#pragma once

#include "../api/FaceMediaPipe.h"

#include <cstdint>

namespace face_mp_internal {

struct NormalizedLandmark
{
    float x;
    float y;
    float z;
};

// Convert MediaPipe-normalized landmarks to the public C ABI contract.
//
// x/y are written in image pixels without clamping so callers retain the
// model's exact estimate. z is preserved unchanged. The face bounding box is
// calculated from every source landmark, even when caller capacity truncates
// the output array, and is clamped to the image bounds.
bool write_face_result(
    const NormalizedLandmark* source,
    int32_t source_count,
    int32_t image_width,
    int32_t image_height,
    FaceMPResult* result);

}  // namespace face_mp_internal
