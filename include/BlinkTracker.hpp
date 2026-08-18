#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

#include <string>
#include <vector>


class BlinkTracker
{
public:

    explicit BlinkTracker(
        const std::string& landmarkModelPath,
        double eyeCloseThreshold = 0.27
    );


    bool process(
        cv::Mat& frame,
        const cv::Rect& faceBox
    );


    int getBlinkCount() const;


    double getEAR() const;


    bool isEyeClosed() const;


private:

    double calculateEAR(
        const std::vector<cv::Point2f>& eyePoints
    ) const;


    cv::Ptr<cv::face::Facemark> facemark_;


    double eyeCloseThreshold_;


    int blinkCount_;


    bool isEyeClosed_;


    double currentEAR_;
};