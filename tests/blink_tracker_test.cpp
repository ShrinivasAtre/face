#include "BlinkTracker.hpp"

#include <opencv2/core.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
std::vector<cv::Point2f> makeLandmarks(float halfEyeHeight)
{
    std::vector<cv::Point2f> points(68, cv::Point2f(100.0f, 100.0f));

    const auto setEye = [&](int start, float xOffset)
    {
        points[start + 0] = cv::Point2f(xOffset + 0.0f, 100.0f);
        points[start + 1] = cv::Point2f(xOffset + 2.0f, 100.0f - halfEyeHeight);
        points[start + 2] = cv::Point2f(xOffset + 8.0f, 100.0f - halfEyeHeight);
        points[start + 3] = cv::Point2f(xOffset + 10.0f, 100.0f);
        points[start + 4] = cv::Point2f(xOffset + 8.0f, 100.0f + halfEyeHeight);
        points[start + 5] = cv::Point2f(xOffset + 2.0f, 100.0f + halfEyeHeight);
    };

    setEye(36, 100.0f);
    setEye(42, 140.0f);
    return points;
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

    std::vector<cv::Point2f> tooShort(47);
    if (!check(!tracker.process(frame, tooShort), "short landmarks should fail") ||
        !check(!tracker.isLandmarkValid(), "short landmarks should be invalid") ||
        !check(!tracker.isEyeClosed(), "invalid landmarks should clear closed state") ||
        !check(near(tracker.getAverageEAR(), 0.0), "invalid landmarks should clear EAR"))
    {
        return 1;
    }

    auto nonFinite = open;
    nonFinite[36].x = std::numeric_limits<float>::quiet_NaN();
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
