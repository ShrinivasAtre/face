#include "BgrToRgb.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>

namespace {

bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << "\n";
        return false;
    }
    return true;
}

bool test_compact_rows()
{
    const std::array<uint8_t, 12> bgr = {
        1, 2, 3, 4, 5, 6,
        7, 8, 9, 10, 11, 12
    };
    std::array<uint8_t, 12> rgb{};

    if (!expect(face_mp_internal::copy_bgr_to_rgb(
            bgr.data(), 2, 2, 6, rgb.data(), 6),
            "compact conversion rejected"))
    {
        return false;
    }

    const std::array<uint8_t, 12> expected = {
        3, 2, 1, 6, 5, 4,
        9, 8, 7, 12, 11, 10
    };
    return expect(rgb == expected, "compact BGR channel order");
}

bool test_padded_rows()
{
    const std::array<uint8_t, 16> bgr = {
        1, 2, 3, 4, 5, 6, 201, 202,
        7, 8, 9, 10, 11, 12, 203, 204
    };
    std::array<uint8_t, 18> rgb;
    rgb.fill(0xEE);

    if (!expect(face_mp_internal::copy_bgr_to_rgb(
            bgr.data(), 2, 2, 8, rgb.data(), 9),
            "padded conversion rejected"))
    {
        return false;
    }

    const std::array<uint8_t, 18> expected = {
        3, 2, 1, 6, 5, 4, 0xEE, 0xEE, 0xEE,
        9, 8, 7, 12, 11, 10, 0xEE, 0xEE, 0xEE
    };
    return expect(rgb == expected, "padded rows or padding modified");
}

bool test_invalid_inputs()
{
    std::array<uint8_t, 12> input{};
    std::array<uint8_t, 12> output{};

    bool ok = true;
    ok &= expect(!face_mp_internal::copy_bgr_to_rgb(
        nullptr, 2, 2, 6, output.data(), 6), "null input accepted");
    ok &= expect(!face_mp_internal::copy_bgr_to_rgb(
        input.data(), 2, 2, 6, nullptr, 6), "null output accepted");
    ok &= expect(!face_mp_internal::copy_bgr_to_rgb(
        input.data(), 0, 2, 6, output.data(), 6), "zero width accepted");
    ok &= expect(!face_mp_internal::copy_bgr_to_rgb(
        input.data(), 2, 0, 6, output.data(), 6), "zero height accepted");
    ok &= expect(!face_mp_internal::copy_bgr_to_rgb(
        input.data(), 2, 2, 5, output.data(), 6), "short input stride accepted");
    ok &= expect(!face_mp_internal::copy_bgr_to_rgb(
        input.data(), 2, 2, 6, output.data(), 5), "short output stride accepted");
    ok &= expect(!face_mp_internal::is_valid_bgr_image(
        input.data(), std::numeric_limits<int32_t>::max(), 1,
        std::numeric_limits<int32_t>::max()),
        "overflowing row size accepted");
    return ok;
}

}  // namespace

int main()
{
    if (!test_compact_rows() ||
        !test_padded_rows() ||
        !test_invalid_inputs())
    {
        return 1;
    }

    std::cout << "BGR to RGB conversion tests PASSED\n";
    return 0;
}
