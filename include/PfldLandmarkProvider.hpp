#pragma once

#include "FaceBackend.hpp"

#include <opencv2/core.hpp>

#include <memory>
#include <string>
#include <vector>

// ONNX PFLD landmark acquisition behind a provider-specific boundary.
// Coordinates returned to callers are image pixels; model preprocessing and
// topology do not escape this component.
class PfldLandmarkProvider
{
public:
    PfldLandmarkProvider();
    ~PfldLandmarkProvider();

    PfldLandmarkProvider(const PfldLandmarkProvider&) = delete;
    PfldLandmarkProvider& operator=(const PfldLandmarkProvider&) = delete;

    bool initialize(const std::string& modelPath);
    bool detect(const cv::Mat& frame, const cv::Rect& faceBox,
                std::vector<FaceLandmark>& result);

    static constexpr std::size_t landmarkCount() { return 68; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

