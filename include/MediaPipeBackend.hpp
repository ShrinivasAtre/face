#pragma once

#include "FaceBackend.hpp"
#include "FaceMediaPipeRuntime.hpp"

#include <filesystem>
#include <vector>

class MediaPipeBackend final : public FaceBackend
{
public:
    explicit MediaPipeBackend(std::filesystem::path libraryPath);
    ~MediaPipeBackend() override;

    bool initialize(const std::string& modelPath) override;
    bool process(const cv::Mat& frame, FaceResult& result) override;
    const char* name() const override;

private:
    std::filesystem::path libraryPath_;
    FaceMediaPipeRuntime runtime_;
    FaceMPHandle* handle_ = nullptr;
    std::vector<FaceMPLandmark> bridgeLandmarks_{478};
};
