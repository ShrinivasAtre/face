#pragma once

#include "EyeLandmarks.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <string>
#include <vector>

class BlinkTracker
{
public:
    BlinkTracker();

    explicit BlinkTracker(
        double earThreshold);

    // Process one frame using backend-neutral semantic eye landmarks.
    // Returns true when the landmarks were valid and processed.
    bool process(
        cv::Mat& frame,
        const SemanticEyeLandmarks& landmarks);

    // Compatibility API used by existing main.cpp.
    double getEAR() const;

    double getRightEAR() const;
    double getLeftEAR() const;
    double getAverageEAR() const;

    int getBlinkCount() const;

    bool isEyeClosed() const;
    bool isLandmarkValid() const;

private:
    double calculateEAR(
        const EyeLandmarks& eye) const;

    void drawEyeLandmarks(
        cv::Mat& frame,
        const EyeLandmarks& eye,
        const std::string& label,
        bool rightEye) const;

private:
    double rightEAR_;
    double leftEAR_;
    double averageEAR_;

    // Recorded calibration values.
    double minRightEAR_;
    double maxRightEAR_;

    double minLeftEAR_;
    double maxLeftEAR_;

    double minAverageEAR_;
    double maxAverageEAR_;

    bool landmarkValid_;
    bool eyeClosed_;

    int blinkCount_;

    double earCloseThreshold_;
};
