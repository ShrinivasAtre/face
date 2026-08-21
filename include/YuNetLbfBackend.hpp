#pragma once

#include "FaceBackend.hpp"

#include <memory>
#include <string>

class FaceDetector;
class LbfLandmarkDetector;

class YuNetLbfBackend final : public FaceBackend
{
public:
    explicit YuNetLbfBackend(std::string lbfModelPath);
    ~YuNetLbfBackend() override;

    bool initialize(const std::string& yunetModelPath) override;
    bool process(const cv::Mat& frame, FaceResult& result) override;
    const char* name() const override;

private:
    std::string lbfModelPath_;
    std::unique_ptr<FaceDetector> faceDetector_;
    std::unique_ptr<LbfLandmarkDetector> landmarkDetector_;
};
