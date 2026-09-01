#include "DmsHeadPoseEstimator.hpp"

#include <opencv2/calib3d.hpp>

#include <array>
#include <cmath>
#include <iostream>

namespace
{
const std::array<cv::Point3d, 6> model = {cv::Point3d{0, 0, 0},          cv::Point3d{0, 330, -65},
                                          cv::Point3d{-225, -170, -135}, cv::Point3d{225, -170, -135},
                                          cv::Point3d{-150, 150, -125},  cv::Point3d{150, 150, -125}};

bool check(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

SemanticFaceGeometry projected(double yawRadians)
{
    const cv::Size size(640, 480);
    const cv::Matx33d camera(640, 0, 320, 0, 640, 240, 0, 0, 1);
    cv::Vec3d rotation(0, yawRadians, 0), translation(0, 0, 1200);
    std::vector<cv::Point2d> points;
    cv::projectPoints(model, rotation, translation, camera, cv::noArray(), points);
    SemanticFaceGeometry value;
    value.noseTip = points[0];
    value.chin = points[1];
    value.rightEyeOuter = points[2];
    value.leftEyeOuter = points[3];
    value.rightMouthCorner = points[4];
    value.leftMouthCorner = points[5];
    value.upperInnerLip = value.rightMouthCorner;
    value.lowerInnerLip = value.leftMouthCorner;
    return value;
}
} // namespace

int main()
{
    HeadPoseAngles result;
    if (!check(estimateHeadPose(projected(0.0), {640, 480}, result), "frontal pose") ||
        !check(std::abs(result.yawDegrees) < 1.0F && std::abs(result.pitchDegrees) < 1.0F, "frontal angles") ||
        !check(result.reprojectionErrorPixels < 0.1F, "frontal reprojection"))
        return 1;
    if (!check(estimateHeadPose(projected(20.0 * CV_PI / 180.0), {640, 480}, result), "turned pose") ||
        !check(std::abs(result.yawDegrees + 20.0F) < 1.0F,
               "OpenCV-positive yaw normalized to semantic left"))
        return 1;
    if (!check(!estimateHeadPose(projected(0.0), {}, result), "invalid frame rejected"))
        return 1;
    using namespace std::chrono_literals;
    HeadPoseNeutralConfig config;
    config.confirmation = 100ms;
    HeadPoseNeutralCalibrator calibrator(config);
    HeadPoseAngles biased;
    biased.yawDegrees = -8.0F;
    biased.pitchDegrees = 18.0F;
    if (!check(!calibrator.update(0ms, biased), "neutral calibration starts") ||
        !check(!calibrator.update(50ms, biased), "neutral calibration waits"))
        return 1;
    const auto calibrated = calibrator.update(100ms, biased);
    if (!check(calibrated && std::abs(calibrated->yawDegrees) < 0.01F && std::abs(calibrated->pitchDegrees) < 0.01F,
               "neutral bias removed"))
        return 1;
    std::cout << "DMS head pose estimator tests PASSED\n";
    return 0;
}
