#pragma once

#include <cstdint>

namespace face_mp_internal {

bool is_valid_bgr_image(
    const uint8_t* bgr,
    int32_t width,
    int32_t height,
    int32_t bgr_stride);

bool copy_bgr_to_rgb(
    const uint8_t* bgr,
    int32_t width,
    int32_t height,
    int32_t bgr_stride,
    uint8_t* rgb,
    int32_t rgb_stride);

}  // namespace face_mp_internal
