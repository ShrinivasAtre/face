#pragma once

#include "DriverIdentity.hpp"

#include <string>

namespace dms
{
enum class EnrollmentDecision
{
    InvalidConfiguration,
    InvalidInput,
    AlignmentUnavailable,
    QualityUnavailable,
    QualityRejected,
    PresentationUnavailable,
    SpoofRejected,
    EmbeddingUnavailable,
    ThresholdApprovalRequired,
    Accepted
};

struct EnrollmentGateConfig
{
    float minimumQuality = 0.0F;
    bool thresholdsApproved = false;
    bool valid(std::string &error) const noexcept;
};

struct EnrollmentEvaluation
{
    EnrollmentDecision decision = EnrollmentDecision::InvalidConfiguration;
    float quality = 0.0F;
    PresentationResult presentation;
    FaceEmbedding embedding;
    std::string diagnostic;

    bool accepted() const noexcept { return decision == EnrollmentDecision::Accepted; }
};

class DriverEnrollmentProcessor
{
  public:
    DriverEnrollmentProcessor(EnrollmentGateConfig config, FaceAlignmentProvider &alignment,
                              FaceQualityProvider &quality, PresentationAttackProvider &presentation,
                              FaceEmbeddingProvider &embedding) noexcept;
    EnrollmentEvaluation evaluate(const FaceImageView &sourceFace);

  private:
    EnrollmentGateConfig config_;
    FaceAlignmentProvider &alignment_;
    FaceQualityProvider &quality_;
    PresentationAttackProvider &presentation_;
    FaceEmbeddingProvider &embedding_;
};
} // namespace dms
