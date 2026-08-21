#include "LandmarkConversion.h"

#include <array>
#include <cmath>
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

bool nearly_equal(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

bool test_scaling_z_and_bbox()
{
    const std::array<face_mp_internal::NormalizedLandmark, 3> source = {{
        {0.25f, 0.20f, -0.10f},
        {0.75f, 0.80f, 0.20f},
        {0.50f, 0.40f, 0.00f},
    }};
    std::array<FaceMPLandmark, 3> output{};
    FaceMPResult result{};
    result.landmarks = output.data();
    result.landmark_capacity = static_cast<int32_t>(output.size());

    bool ok = face_mp_internal::write_face_result(
        source.data(), static_cast<int32_t>(source.size()),
        200, 100, &result);

    return expect(ok, "valid landmarks rejected") &&
        expect(result.detected == 1, "face not marked detected") &&
        expect(result.landmark_count == 3, "wrong landmark count") &&
        expect(nearly_equal(output[0].x, 50.0f), "x pixel scaling") &&
        expect(nearly_equal(output[0].y, 20.0f), "y pixel scaling") &&
        expect(nearly_equal(output[0].z, -0.10f), "z was not preserved") &&
        expect(result.face_x == 50 && result.face_y == 20,
            "bbox origin rounding") &&
        expect(result.face_width == 100 && result.face_height == 60,
            "bbox extent rounding");
}

bool test_capacity_uses_full_face_bbox()
{
    const std::array<face_mp_internal::NormalizedLandmark, 3> source = {{
        {0.40f, 0.40f, 0.0f},
        {0.50f, 0.50f, 0.0f},
        {0.90f, 0.80f, 0.0f},
    }};
    std::array<FaceMPLandmark, 1> output{};
    FaceMPResult result{};
    result.landmarks = output.data();
    result.landmark_capacity = 1;

    bool ok = face_mp_internal::write_face_result(
        source.data(), 3, 100, 100, &result);

    return expect(ok, "truncated output rejected") &&
        expect(result.landmark_count == 1, "capacity not honored") &&
        expect(result.face_x == 40 && result.face_y == 40,
            "truncated bbox origin") &&
        expect(result.face_width == 50 && result.face_height == 40,
            "bbox did not use all source landmarks");
}

bool test_outside_points_and_bbox_clamping()
{
    const std::array<face_mp_internal::NormalizedLandmark, 2> source = {{
        {-0.10f, 0.25f, -1.0f},
        {1.20f, 1.10f, 1.0f},
    }};
    std::array<FaceMPLandmark, 2> output{};
    FaceMPResult result{};
    result.landmarks = output.data();
    result.landmark_capacity = 2;

    bool ok = face_mp_internal::write_face_result(
        source.data(), 2, 100, 80, &result);

    return expect(ok, "outside landmarks rejected") &&
        expect(nearly_equal(output[0].x, -10.0f),
            "landmark x should remain unclamped") &&
        expect(nearly_equal(output[1].y, 88.0f),
            "landmark y should remain unclamped") &&
        expect(result.face_x == 0 && result.face_y == 20,
            "clamped bbox origin") &&
        expect(result.face_width == 100 && result.face_height == 60,
            "bbox not clamped to image");
}

bool test_empty_result_resets_state()
{
    std::array<FaceMPLandmark, 1> output{};
    FaceMPResult result{};
    result.detected = 1;
    result.face_x = 7;
    result.face_y = 8;
    result.face_width = 9;
    result.face_height = 10;
    result.landmark_count = 1;
    result.landmarks = output.data();
    result.landmark_capacity = 1;

    bool ok = face_mp_internal::write_face_result(
        nullptr, 0, 100, 80, &result);

    return expect(ok, "empty result rejected") &&
        expect(result.detected == 0, "empty result detected") &&
        expect(result.landmark_count == 0, "empty landmark count") &&
        expect(result.face_x == 0 && result.face_y == 0 &&
            result.face_width == 0 && result.face_height == 0,
            "empty bbox not reset");
}

bool test_invalid_inputs()
{
    const face_mp_internal::NormalizedLandmark valid{0.5f, 0.5f, 0.0f};
    std::array<FaceMPLandmark, 1> output{};
    FaceMPResult result{};
    result.landmarks = output.data();
    result.landmark_capacity = 1;

    bool ok = true;
    ok &= expect(!face_mp_internal::write_face_result(
        &valid, 1, 0, 80, &result), "zero width accepted");
    ok &= expect(!face_mp_internal::write_face_result(
        &valid, 1, 100, 0, &result), "zero height accepted");
    ok &= expect(!face_mp_internal::write_face_result(
        nullptr, 1, 100, 80, &result), "null source accepted");
    ok &= expect(!face_mp_internal::write_face_result(
        &valid, -1, 100, 80, &result), "negative count accepted");

    FaceMPResult no_storage{};
    ok &= expect(!face_mp_internal::write_face_result(
        &valid, 1, 100, 80, &no_storage), "missing storage accepted");

    const face_mp_internal::NormalizedLandmark nan_value{
        std::numeric_limits<float>::quiet_NaN(), 0.5f, 0.0f};
    ok &= expect(!face_mp_internal::write_face_result(
        &nan_value, 1, 100, 80, &result), "NaN accepted");
    ok &= expect(result.detected == 0 && result.landmark_count == 0,
        "failure did not reset result");
    return ok;
}

}  // namespace

int main()
{
    if (!test_scaling_z_and_bbox() ||
        !test_capacity_uses_full_face_bbox() ||
        !test_outside_points_and_bbox_clamping() ||
        !test_empty_result_resets_state() ||
        !test_invalid_inputs())
    {
        return 1;
    }

    std::cout << "Landmark conversion tests PASSED\n";
    return 0;
}
