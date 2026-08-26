#pragma once

#include "FaceBackend.hpp"

#include <memory>
#include <string>

class FaceDetector;
class PfldLandmarkProvider;

class YuNetPfldBackend final : public FaceBackend
{
public:
    explicit YuNetPfldBackend(std::string pfldModelPath);
    ~YuNetPfldBackend() override;

    bool initialize(const std::string& modelPaths) override;
    bool process(const cv::Mat& frame, FaceResult& result) override;
    const char* name() const override;

private:
    std::string pfldModelPath_;
    std::unique_ptr<FaceDetector> faceDetector_;
    std::unique_ptr<PfldLandmarkProvider> landmarkProvider_;
};
