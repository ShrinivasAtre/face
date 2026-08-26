#include "YuNetPfldBackend.hpp"

#include "FaceDetector.hpp"
#include "PfldLandmarkProvider.hpp"

#include <exception>
#include <iostream>
#include <utility>

YuNetPfldBackend::YuNetPfldBackend(std::string pfldModelPath)
    : pfldModelPath_(std::move(pfldModelPath)) {}
YuNetPfldBackend::~YuNetPfldBackend() = default;

bool YuNetPfldBackend::initialize(const std::string& yunetModelPath)
{
    faceDetector_.reset();
    landmarkProvider_.reset();
    try
    {
        auto detector = std::make_unique<FaceDetector>(
            yunetModelPath, cv::Size(320, 320), 0.60f, 0.3f, 5000);
        auto landmarks = std::make_unique<PfldLandmarkProvider>();
        if (!landmarks->initialize(pfldModelPath_))
            return false;
        faceDetector_ = std::move(detector);
        landmarkProvider_ = std::move(landmarks);
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ERROR: YuNet/PFLD initialization failed: "
                  << error.what() << '\n';
        return false;
    }
}

bool YuNetPfldBackend::process(const cv::Mat& frame, FaceResult& result)
{
    result = {};
    if (!faceDetector_ || !landmarkProvider_ || frame.empty())
        return false;
    if (!faceDetector_->detect(frame, result.faceBox))
        return true;
    result.detected = true;
    result.landmarksValid = landmarkProvider_->detect(
        frame, result.faceBox, result.landmarks);
    return true;
}

const char* YuNetPfldBackend::name() const { return "pfld"; }
