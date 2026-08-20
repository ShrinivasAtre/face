#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

#include <string>
#include <vector>

class BlinkTracker
{
public:
    BlinkTracker();

    // Constructor compatible with existing main.cpp.
    BlinkTracker(
        const std::string& modelPath,
        double earThreshold);

    bool initialize(
        const std::string& modelPath);

    // Process one frame using the supplied face bounding box.
    // Returns true when the current eye state is considered closed.
    bool process(
        cv::Mat& frame,
        const cv::Rect& faceBox);

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
        const std::vector<cv::Point2f>& eyePoints) const;

    void drawEyeLandmarks(
        cv::Mat& frame,
        const std::vector<cv::Point2f>& eyePoints,
        const std::string& label,
        bool rightEye) const;

private:
    cv::Ptr<cv::face::Facemark> facemark_;

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