#include "BlinkTracker.hpp"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>


BlinkTracker::BlinkTracker()
    : rightEAR_(0.0),
      leftEAR_(0.0),
      averageEAR_(0.0),

      minRightEAR_(std::numeric_limits<double>::max()),
      maxRightEAR_(std::numeric_limits<double>::lowest()),

      minLeftEAR_(std::numeric_limits<double>::max()),
      maxLeftEAR_(std::numeric_limits<double>::lowest()),

      minAverageEAR_(std::numeric_limits<double>::max()),
      maxAverageEAR_(std::numeric_limits<double>::lowest()),

      landmarkValid_(false),
      eyeClosed_(false),
      blinkCount_(0),
      earCloseThreshold_(0.27)
{
}


BlinkTracker::BlinkTracker(
    double earThreshold)
    : rightEAR_(0.0),
      leftEAR_(0.0),
      averageEAR_(0.0),

      minRightEAR_(std::numeric_limits<double>::max()),
      maxRightEAR_(std::numeric_limits<double>::lowest()),

      minLeftEAR_(std::numeric_limits<double>::max()),
      maxLeftEAR_(std::numeric_limits<double>::lowest()),

      minAverageEAR_(std::numeric_limits<double>::max()),
      maxAverageEAR_(std::numeric_limits<double>::lowest()),

      landmarkValid_(false),
      eyeClosed_(false),
      blinkCount_(0),
      earCloseThreshold_(earThreshold)
{
}


double BlinkTracker::calculateEAR(
    const EyeLandmarks& eye) const
{
    const double vertical1 =
        cv::norm(
            eye.upperOuterLid -
            eye.lowerOuterLid);

    const double vertical2 =
        cv::norm(
            eye.upperInnerLid -
            eye.lowerInnerLid);

    const double horizontal =
        cv::norm(
            eye.outerCorner -
            eye.innerCorner);

    if (horizontal < 1e-6)
        return 0.0;

    return
        (vertical1 + vertical2)
        / (2.0 * horizontal);
}


void BlinkTracker::drawEyeLandmarks(
    cv::Mat& frame,
    const EyeLandmarks& eye,
    const std::string& label,
    bool rightEye) const
{
    const cv::Point2f eyePoints[] = {
        eye.outerCorner,
        eye.upperOuterLid,
        eye.upperInnerLid,
        eye.innerCorner,
        eye.lowerInnerLid,
        eye.lowerOuterLid};

    const cv::Scalar pointColor =
        rightEye
            ? cv::Scalar(0, 255, 255)
            : cv::Scalar(255, 255, 0);

    const cv::Scalar lineColor =
        rightEye
            ? cv::Scalar(0, 200, 200)
            : cv::Scalar(200, 200, 0);

    for (size_t i = 0;
         i < 6;
         ++i)
    {
        const cv::Point2f& p =
            eyePoints[i];

        cv::circle(
            frame,
            p,
            3,
            pointColor,
            -1);

        cv::putText(
            frame,
            label + std::to_string(i),
            cv::Point(
                static_cast<int>(p.x) + 4,
                static_cast<int>(p.y) - 4),
            cv::FONT_HERSHEY_SIMPLEX,
            0.4,
            pointColor,
            1,
            cv::LINE_AA);
    }

    for (int i = 0; i < 6; ++i)
    {
        cv::line(
            frame,
            eyePoints[i],
            eyePoints[(i + 1) % 6],
            lineColor,
            1,
            cv::LINE_AA);
    }
}


bool BlinkTracker::process(
    cv::Mat& frame,
    const SemanticEyeLandmarks& landmarks)
{
    landmarkValid_ = false;

    rightEAR_ = 0.0;
    leftEAR_ = 0.0;
    averageEAR_ = 0.0;

    if (frame.empty())
    {
        eyeClosed_ = false;
        return false;
    }

    const cv::Point2f points[] = {
        landmarks.rightEye.outerCorner,
        landmarks.rightEye.upperOuterLid,
        landmarks.rightEye.upperInnerLid,
        landmarks.rightEye.innerCorner,
        landmarks.rightEye.lowerInnerLid,
        landmarks.rightEye.lowerOuterLid,
        landmarks.leftEye.outerCorner,
        landmarks.leftEye.upperOuterLid,
        landmarks.leftEye.upperInnerLid,
        landmarks.leftEye.innerCorner,
        landmarks.leftEye.lowerInnerLid,
        landmarks.leftEye.lowerOuterLid};

    for (const cv::Point2f& point : points)
    {
        if (!std::isfinite(point.x) || !std::isfinite(point.y))
        {
            eyeClosed_ = false;
            return false;
        }
    }

    rightEAR_ =
        calculateEAR(landmarks.rightEye);

    leftEAR_ =
        calculateEAR(landmarks.leftEye);

    averageEAR_ =
        (rightEAR_ + leftEAR_) / 2.0;

    landmarkValid_ = true;


    // ------------------------------------------------------------
    // Update recorded minimum / maximum values.
    // ------------------------------------------------------------

    minRightEAR_ =
        std::min(minRightEAR_, rightEAR_);

    maxRightEAR_ =
        std::max(maxRightEAR_, rightEAR_);

    minLeftEAR_ =
        std::min(minLeftEAR_, leftEAR_);

    maxLeftEAR_ =
        std::max(maxLeftEAR_, leftEAR_);

    minAverageEAR_ =
        std::min(minAverageEAR_, averageEAR_);

    maxAverageEAR_ =
        std::max(maxAverageEAR_, averageEAR_);


    // Draw eye landmarks.
    drawEyeLandmarks(
        frame,
        landmarks.rightEye,
        "R",
        true);

    drawEyeLandmarks(
        frame,
        landmarks.leftEye,
        "L",
        false);


    // ------------------------------------------------------------
    // Simple diagnostic blink logic.
    // ------------------------------------------------------------

    const bool currentClosed =
        rightEAR_ < earCloseThreshold_ ||
        leftEAR_ < earCloseThreshold_;

    if (currentClosed)
    {
        if (!eyeClosed_)
        {
            ++blinkCount_;
        }

        eyeClosed_ = true;
    }
    else
    {
        eyeClosed_ = false;
    }


    // ------------------------------------------------------------
    // Diagnostic information.
    //
    // Displayed at the bottom of the frame.
    // ------------------------------------------------------------

    const int h = frame.rows;

    const int line1Y = h - 150;
    const int line2Y = h - 120;
    const int line3Y = h - 90;
    const int line4Y = h - 60;
    const int line5Y = h - 30;


    // Right EAR
    cv::putText(
        frame,
        "R-EAR: " +
            cv::format("%.3f", rightEAR_) +
            "  MIN: " +
            cv::format("%.3f", minRightEAR_) +
            "  MAX: " +
            cv::format("%.3f", maxRightEAR_),
        cv::Point(30, line1Y),
        cv::FONT_HERSHEY_SIMPLEX,
        0.60,
        cv::Scalar(0, 255, 255),
        2,
        cv::LINE_AA);


    // Left EAR
    cv::putText(
        frame,
        "L-EAR: " +
            cv::format("%.3f", leftEAR_) +
            "  MIN: " +
            cv::format("%.3f", minLeftEAR_) +
            "  MAX: " +
            cv::format("%.3f", maxLeftEAR_),
        cv::Point(30, line2Y),
        cv::FONT_HERSHEY_SIMPLEX,
        0.60,
        cv::Scalar(255, 255, 0),
        2,
        cv::LINE_AA);


    // Average EAR
    cv::putText(
        frame,
        "AVG:   " +
            cv::format("%.3f", averageEAR_) +
            "  MIN: " +
            cv::format("%.3f", minAverageEAR_) +
            "  MAX: " +
            cv::format("%.3f", maxAverageEAR_),
        cv::Point(30, line3Y),
        cv::FONT_HERSHEY_SIMPLEX,
        0.60,
        cv::Scalar(255, 255, 255),
        2,
        cv::LINE_AA);


    // Blink count
    cv::putText(
        frame,
        "Blinks: " +
            std::to_string(blinkCount_),
        cv::Point(30, line4Y),
        cv::FONT_HERSHEY_SIMPLEX,
        0.60,
        cv::Scalar(255, 255, 255),
        2,
        cv::LINE_AA);


    // Eye state
    cv::putText(
        frame,
        eyeClosed_
            ? "STATE: CLOSED"
            : "STATE: OPEN",
        cv::Point(300, line4Y),
        cv::FONT_HERSHEY_SIMPLEX,
        0.60,
        eyeClosed_
            ? cv::Scalar(0, 0, 255)
            : cv::Scalar(0, 255, 0),
        2,
        cv::LINE_AA);


    // Calibration reminder.
    cv::putText(
        frame,
        "Min/Max recorded since application start",
        cv::Point(30, line5Y),
        cv::FONT_HERSHEY_SIMPLEX,
        0.50,
        cv::Scalar(200, 200, 200),
        1,
        cv::LINE_AA);


    return true;
}


// ------------------------------------------------------------
// Compatibility API
// ------------------------------------------------------------

double BlinkTracker::getEAR() const
{
    return averageEAR_;
}


// ------------------------------------------------------------
// Public getters
// ------------------------------------------------------------

int BlinkTracker::getBlinkCount() const
{
    return blinkCount_;
}


double BlinkTracker::getRightEAR() const
{
    return rightEAR_;
}


double BlinkTracker::getLeftEAR() const
{
    return leftEAR_;
}


double BlinkTracker::getAverageEAR() const
{
    return averageEAR_;
}


bool BlinkTracker::isEyeClosed() const
{
    return eyeClosed_;
}


bool BlinkTracker::isLandmarkValid() const
{
    return landmarkValid_;
}
