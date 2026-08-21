#include "BlinkTracker.hpp"

#include <opencv2/core.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
EyeLandmarks makeEye(float halfEyeHeight, float xOffset)
{
    return {
        cv::Point2f(xOffset + 0.0f, 100.0f),
        cv::Point2f(xOffset + 2.0f, 100.0f - halfEyeHeight),
        cv::Point2f(xOffset + 8.0f, 100.0f - halfEyeHeight),
        cv::Point2f(xOffset + 10.0f, 100.0f),
        cv::Point2f(xOffset + 8.0f, 100.0f + halfEyeHeight),
        cv::Point2f(xOffset + 2.0f, 100.0f + halfEyeHeight)};
}

SemanticEyeLandmarks makeLandmarks(float halfEyeHeight)
{
    return {
        makeEye(halfEyeHeight, 100.0f),
        makeEye(halfEyeHeight, 140.0f)};
}

bool near(double actual, double expected)
{
    return std::abs(actual - expected) < 1e-6;
}

bool check(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << std::endl;
    return condition;
}
}  // namespace

int main()
{
    cv::Mat frame = cv::Mat::zeros(240, 320, CV_8UC3);
    BlinkTracker tracker(0.27);

    const auto open = makeLandmarks(2.0f);
    const auto closed = makeLandmarks(0.5f);

    if (!check(tracker.process(frame, open), "open landmarks should process") ||
        !check(tracker.isLandmarkValid(), "open landmarks should be valid") ||
        !check(!tracker.isEyeClosed(), "open eyes should not be closed") ||
        !check(near(tracker.getRightEAR(), 0.4), "right open EAR") ||
        !check(near(tracker.getLeftEAR(), 0.4), "left open EAR") ||
        !check(tracker.getBlinkCount() == 0, "initial blink count"))
    {
        return 1;
    }

    if (!check(tracker.process(frame, closed), "closed landmarks should process") ||
        !check(tracker.isEyeClosed(), "closed eyes should be closed") ||
        !check(tracker.getBlinkCount() == 1, "open-to-closed counts once") ||
        !check(tracker.process(frame, closed), "repeated closed landmarks should process") ||
        !check(tracker.getBlinkCount() == 1, "repeated closed frame should not recount"))
    {
        return 1;
    }

    if (!check(tracker.process(frame, open), "reopened landmarks should process") ||
        !check(!tracker.isEyeClosed(), "eyes should reopen") ||
        !check(tracker.process(frame, closed), "second closure should process") ||
        !check(tracker.getBlinkCount() == 2, "second transition should count"))
    {
        return 1;
    }

    auto nonFinite = open;
    nonFinite.rightEye.outerCorner.x =
        std::numeric_limits<float>::quiet_NaN();
    if (!check(!tracker.process(frame, nonFinite), "non-finite landmarks should fail") ||
        !check(tracker.process(frame, open), "valid landmarks should recover") ||
        !check(tracker.isLandmarkValid(), "recovered landmarks should be valid") ||
        !check(!tracker.isEyeClosed(), "recovered eyes should be open"))
    {
        return 1;
    }

    std::cout << "Blink tracker tests PASSED" << std::endl;
    return 0;
}
