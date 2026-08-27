#include "BackendFaceGeometryMapper.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
} // namespace

int main()
{
    FaceResult ibug;
    ibug.detected = ibug.landmarksValid = true;
    ibug.landmarks.resize(68);
    ibug.landmarks[30] = {50, 40, 0};
    ibug.landmarks[8] = {50, 90, 0};
    ibug.landmarks[36] = {30, 30, 0};
    ibug.landmarks[45] = {70, 30, 0};
    ibug.landmarks[48] = {30, 60, 0};
    ibug.landmarks[54] = {70, 60, 0};
    ibug.landmarks[62] = {50, 55, 0};
    ibug.landmarks[66] = {50, 65, 0};
    SemanticFaceGeometry geometry;
    const auto lbfOpen = [&]() {
        if (!mapBackendFaceGeometry(BackendKind::YuNet, ibug, geometry))
            return std::optional<float>{};
        return calculateMouthOpenness(geometry);
    }();
    if (!check(lbfOpen && std::abs(*lbfOpen - 0.5F) < 0.001F, "iBUG mouth openness"))
        return 1;

    FaceResult mediaPipe;
    mediaPipe.detected = mediaPipe.landmarksValid = true;
    mediaPipe.landmarks.resize(478);
    mediaPipe.landmarks[1] = {50, 40, 0};
    mediaPipe.landmarks[152] = {50, 90, 0};
    mediaPipe.landmarks[33] = {30, 30, 0};
    mediaPipe.landmarks[263] = {70, 30, 0};
    mediaPipe.landmarks[61] = {30, 60, 0};
    mediaPipe.landmarks[291] = {70, 60, 0};
    mediaPipe.landmarks[13] = {50, 55, 0};
    mediaPipe.landmarks[14] = {50, 65, 0};
    if (!check(mapBackendFaceGeometry(BackendKind::MediaPipe, mediaPipe, geometry), "MediaPipe semantic mapping") ||
        !check(calculateMouthOpenness(geometry).has_value(), "MediaPipe mouth openness"))
        return 1;

    mediaPipe.landmarks[14].x = std::numeric_limits<float>::quiet_NaN();
    if (!check(!mapBackendFaceGeometry(BackendKind::MediaPipe, mediaPipe, geometry), "non-finite geometry rejected"))
        return 1;
    ibug.landmarks.resize(20);
    if (!check(!mapBackendFaceGeometry(BackendKind::Pfld, ibug, geometry), "short topology rejected"))
        return 1;
    std::cout << "Backend face geometry mapper tests PASSED\n";
    return 0;
}
