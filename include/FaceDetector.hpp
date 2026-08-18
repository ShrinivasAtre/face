#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

#include <string>


class FaceDetector
{
public:

    FaceDetector(
        const std::string& modelPath,
        const cv::Size& inputSize,
        float scoreThreshold = 0.60f,
        float nmsThreshold = 0.3f,
        int topK = 5000
    );


    bool detect(
        const cv::Mat& frame,
        cv::Rect& faceBox
    );


private:

    cv::Ptr<cv::FaceDetectorYN> detector_;
};