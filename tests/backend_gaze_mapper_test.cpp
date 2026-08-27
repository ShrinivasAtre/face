#include "BackendGazeMapper.hpp"

#include <cmath>
#include <iostream>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

void setIris(FaceResult &face, std::size_t first, float x, float y)
{
    for (std::size_t index = first; index < first + 5; ++index)
        face.landmarks[index] = {x, y, 0};
}
} // namespace

int main()
{
    FaceResult face;
    face.detected = face.landmarksValid = true;
    face.landmarks.resize(478);
    face.landmarks[33] = {20, 40, 0};
    face.landmarks[133] = {40, 40, 0};
    face.landmarks[159] = {30, 35, 0};
    face.landmarks[145] = {30, 45, 0};
    face.landmarks[263] = {80, 40, 0};
    face.landmarks[362] = {60, 40, 0};
    face.landmarks[386] = {70, 35, 0};
    face.landmarks[374] = {70, 45, 0};
    setIris(face, 468, 30, 40);
    setIris(face, 473, 70, 40);
    SemanticGaze gaze;
    if (!check(mapBackendGaze(BackendKind::MediaPipe, face, gaze), "neutral gaze maps") ||
        !check(std::abs(gaze.horizontal) < 0.001F && std::abs(gaze.vertical) < 0.001F, "neutral gaze values") ||
        !check(gaze.interEyeAgreement > 0.99F, "neutral eye agreement"))
        return 1;
    setIris(face, 468, 35, 37);
    setIris(face, 473, 75, 37);
    if (!check(mapBackendGaze(BackendKind::MediaPipe, face, gaze), "shifted gaze maps") ||
        !check(gaze.horizontal < -0.4F && gaze.vertical < -0.4F, "shifted gaze direction"))
        return 1;
    if (!check(!mapBackendGaze(BackendKind::YuNet, face, gaze), "provider without iris is unavailable"))
        return 1;
    std::cout << "Backend gaze mapper tests PASSED\n";
    return 0;
}
