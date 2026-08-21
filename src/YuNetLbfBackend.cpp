#include "YuNetLbfBackend.hpp"

#include "FaceDetector.hpp"
#include "LbfLandmarkDetector.hpp"

#include <opencv2/core.hpp>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

YuNetLbfBackend::YuNetLbfBackend(std::string lbfModelPath)
    : lbfModelPath_(std::move(lbfModelPath)) {}

YuNetLbfBackend::~YuNetLbfBackend() = default;

bool YuNetLbfBackend::initialize(const std::string& yunetModelPath)
{
    faceDetector_.reset();
    landmarkDetector_.reset();
    try
    {
        auto detector = std::make_unique<FaceDetector>(
            yunetModelPath, cv::Size(320, 320), 0.60f, 0.3f, 5000);
        auto landmarks = std::make_unique<LbfLandmarkDetector>();
        if (!landmarks->initialize(lbfModelPath_))
        {
            return false;
        }
        faceDetector_ = std::move(detector);
        landmarkDetector_ = std::move(landmarks);
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ERROR: YuNet/LBF initialization failed: "
                  << error.what() << '\n';
        return false;
    }
}

bool YuNetLbfBackend::process(const cv::Mat& frame, FaceResult& result)
{
    result = {};
    if (!faceDetector_ || !landmarkDetector_ || frame.empty())
    {
        return false;
    }
    cv::Rect faceBox;
    if (!faceDetector_->detect(frame, faceBox))
    {
        return true;
    }
    result.detected = true;
    result.faceBox = faceBox;

    std::vector<cv::Point2f> landmarks;
    if (!landmarkDetector_->detect(frame, faceBox, landmarks))
    {
        return true;
    }
    result.landmarks.reserve(landmarks.size());
    for (const cv::Point2f& landmark : landmarks)
    {
        result.landmarks.push_back({landmark.x, landmark.y, 0.0f});
    }
    result.landmarksValid = !result.landmarks.empty();
    return true;
}

const char* YuNetLbfBackend::name() const { return "yunet"; }
