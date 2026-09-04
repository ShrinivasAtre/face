#pragma once

#include "DriverIdentity.hpp"

#include <memory>
#include <string>

namespace dms
{
class OpenCvSFaceProvider final : public FaceAlignmentProvider, public FaceEmbeddingProvider
{
  public:
    OpenCvSFaceProvider(const std::string &detectorPath, const std::string &recognizerPath,
                        std::string modelId) noexcept;
    ~OpenCvSFaceProvider() override;
    OpenCvSFaceProvider(const OpenCvSFaceProvider &) = delete;
    OpenCvSFaceProvider &operator=(const OpenCvSFaceProvider &) = delete;
    bool valid() const noexcept;
    const std::string &diagnostic() const noexcept;
    const std::string &modelId() const noexcept override;
    FaceAlignmentResult align(const FaceImageView &sourceFace) override;
    EmbeddingResult extract(const FaceImageView &alignedFace) override;
  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class OpenCvDiagnosticFaceQualityProvider final : public FaceQualityProvider
{
  public:
    FaceQualityResult assess(const FaceImageView &alignedFace) override;
};

struct OpenCvPadConfig
{
    float liveProbability = 0.8F;
    float spoofProbability = 0.8F;
    bool thresholdsApproved = false;
};

class OpenCvAntiSpoofProvider final : public PresentationAttackProvider
{
  public:
    OpenCvAntiSpoofProvider(const std::string &modelPath, std::string modelId,
                            OpenCvPadConfig config = {}) noexcept;
    ~OpenCvAntiSpoofProvider() override;
    OpenCvAntiSpoofProvider(const OpenCvAntiSpoofProvider &) = delete;
    OpenCvAntiSpoofProvider &operator=(const OpenCvAntiSpoofProvider &) = delete;
    bool valid() const noexcept;
    const std::string &diagnostic() const noexcept;
    const std::string &modelId() const noexcept override;
    PresentationResult evaluate(const FaceImageView &face) override;
  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace dms
