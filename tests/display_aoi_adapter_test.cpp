#include "DisplayAoiAdapter.hpp"

#include <iostream>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    using namespace dms;
    const cv::Size frame(640, 480);
    if (!check(selectDisplayAoi(frame, DisplayFocus::Full, false, {}, nullptr, nullptr).available,
               "full frame is always available")) return 1;
    const auto face = selectDisplayAoi(frame, DisplayFocus::Face, true,
                                       {100, 80, 200, 240}, nullptr, nullptr);
    if (!check(face.available && face.sourceRectangle == cv::Rect(100, 80, 200, 240),
               "face focus uses detected face")) return 1;
    if (!check(!selectDisplayAoi(frame, DisplayFocus::Face, false, {}, nullptr, nullptr).available,
               "missing face produces privacy-safe unavailable state")) return 1;

    SemanticEyeLandmarks eyes;
    eyes.rightEye = {{200.0F, 190.0F}, {201.0F, 188.0F}, {203.0F, 188.0F},
                     {205.0F, 190.0F}, {203.0F, 192.0F}, {201.0F, 192.0F}};
    eyes.leftEye = {{300.0F, 192.0F}, {301.0F, 190.0F}, {303.0F, 190.0F},
                    {305.0F, 192.0F}, {303.0F, 194.0F}, {301.0F, 194.0F}};
    const auto eyeAoi = selectDisplayAoi(frame, DisplayFocus::Eyes, true, {}, &eyes, nullptr);
    if (!check(eyeAoi.available && eyeAoi.sourceRectangle.contains({200, 190}) &&
               eyeAoi.sourceRectangle.contains({304, 192}), "eye focus covers both eyes")) return 1;

    SemanticFaceGeometry geometry;
    geometry.rightMouthCorner = {240.0F, 300.0F};
    geometry.leftMouthCorner = {340.0F, 300.0F};
    geometry.upperInnerLip = {290.0F, 295.0F};
    geometry.lowerInnerLip = {290.0F, 315.0F};
    const auto mouth = selectDisplayAoi(frame, DisplayFocus::Mouth, true, {}, nullptr, &geometry);
    if (!check(mouth.available && mouth.sourceRectangle.contains({290, 305}),
               "mouth focus covers semantic mouth")) return 1;

    std::cout << "Display AOI adapter tests PASSED\n";
    return 0;
}
