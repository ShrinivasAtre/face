#include "LbfLandmarkDetector.hpp"

#include <iostream>
#include <utility>

LbfLandmarkDetector::LbfLandmarkDetector(
    const std::string& modelPath)
{
    initialize(modelPath);
}

bool LbfLandmarkDetector::initialize(
    const std::string& modelPath)
{
    std::cout
        << "Loading LBF landmark model: "
        << modelPath
        << std::endl;

    try
    {
        facemark_ = cv::face::FacemarkLBF::create();
        facemark_->loadModel(modelPath);
    }
    catch (const cv::Exception& e)
    {
        std::cerr
            << "ERROR: Failed to load LBF model: "
            << e.what()
            << std::endl;

        facemark_.release();
        return false;
    }

    std::cout
        << "LBF landmark detector initialized."
        << std::endl;

    return true;
}

bool LbfLandmarkDetector::detect(
    const cv::Mat& frame,
    const cv::Rect& faceBox,
    std::vector<cv::Point2f>& landmarks)
{
    landmarks.clear();

    if (!facemark_ || frame.empty() ||
        faceBox.width <= 0 || faceBox.height <= 0)
    {
        return false;
    }

    std::vector<cv::Rect> boxes{faceBox};
    std::vector<std::vector<cv::Point2f>> detectedLandmarks;

    try
    {
        if (!facemark_->fit(frame, boxes, detectedLandmarks) ||
            detectedLandmarks.empty())
        {
            return false;
        }
    }
    catch (const cv::Exception& e)
    {
        std::cerr
            << "LBF fit error: "
            << e.what()
            << std::endl;

        return false;
    }

    landmarks = std::move(detectedLandmarks.front());
    return true;
}
