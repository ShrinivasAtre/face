#include "BgrToRgb.h"

#include <cstddef>
#include <cstdint>

namespace face_mp_internal {
namespace {

bool valid_dimensions_and_stride(
    int32_t width,
    int32_t height,
    int32_t stride)
{
    if (width <= 0 || height <= 0 || stride <= 0)
    {
        return false;
    }

    const int64_t row_bytes = static_cast<int64_t>(width) * 3;
    return row_bytes <= INT32_MAX && stride >= row_bytes;
}

}  // namespace

bool is_valid_bgr_image(
    const uint8_t* bgr,
    int32_t width,
    int32_t height,
    int32_t bgr_stride)
{
    return bgr != nullptr &&
        valid_dimensions_and_stride(width, height, bgr_stride);
}

bool copy_bgr_to_rgb(
    const uint8_t* bgr,
    int32_t width,
    int32_t height,
    int32_t bgr_stride,
    uint8_t* rgb,
    int32_t rgb_stride)
{
    if (!is_valid_bgr_image(bgr, width, height, bgr_stride) ||
        rgb == nullptr ||
        !valid_dimensions_and_stride(width, height, rgb_stride))
    {
        return false;
    }

    for (int32_t y = 0; y < height; ++y)
    {
        const uint8_t* src =
            bgr + static_cast<size_t>(y) * static_cast<size_t>(bgr_stride);
        uint8_t* dst =
            rgb + static_cast<size_t>(y) * static_cast<size_t>(rgb_stride);

        for (int32_t x = 0; x < width; ++x)
        {
            dst[3 * x + 0] = src[3 * x + 2];
            dst[3 * x + 1] = src[3 * x + 1];
            dst[3 * x + 2] = src[3 * x + 0];
        }
    }

    return true;
}

}  // namespace face_mp_internal
