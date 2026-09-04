#include "DriverEnrollmentProcessor.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace dms
{
namespace
{
bool validView(const FaceImageView &view) noexcept
{
    if (!view.bgr || view.width <= 0 || view.height <= 0 || view.strideBytes <= 0)
        return false;
    if (view.width > std::numeric_limits<int>::max() / 3 || view.strideBytes < view.width * 3)
        return false;
    return true;
}

EnrollmentEvaluation failure(EnrollmentDecision decision, std::string diagnostic)
{
    EnrollmentEvaluation result;
    result.decision = decision;
    result.diagnostic = std::move(diagnostic);
    return result;
}
} // namespace

bool EnrollmentGateConfig::valid(std::string &error) const noexcept
{
    if (!std::isfinite(minimumQuality) || minimumQuality < 0.0F || minimumQuality > 1.0F)
    {
        error = "minimum enrollment quality must be finite and within [0,1]";
        return false;
    }
    error.clear();
    return true;
}

DriverEnrollmentProcessor::DriverEnrollmentProcessor(EnrollmentGateConfig config,
                                                     FaceAlignmentProvider &alignment,
                                                     FaceQualityProvider &quality,
                                                     PresentationAttackProvider &presentation,
                                                     FaceEmbeddingProvider &embedding) noexcept
    : config_(config), alignment_(alignment), quality_(quality), presentation_(presentation),
      embedding_(embedding)
{
}

EnrollmentEvaluation DriverEnrollmentProcessor::evaluate(const FaceImageView &sourceFace)
{
    std::string error;
    if (!config_.valid(error)) return failure(EnrollmentDecision::InvalidConfiguration, error);
    if (!validView(sourceFace)) return failure(EnrollmentDecision::InvalidInput, "invalid source face image");

    auto alignment = alignment_.align(sourceFace);
    if (!alignment.available || !validView(alignment.alignedFace.view()))
        return failure(EnrollmentDecision::AlignmentUnavailable,
                       alignment.diagnostic.empty() ? "face alignment unavailable" : alignment.diagnostic);

    auto quality = quality_.assess(alignment.alignedFace.view());
    if (!quality.available || !std::isfinite(quality.score) || quality.score < 0.0F || quality.score > 1.0F)
        return failure(EnrollmentDecision::QualityUnavailable,
                       quality.diagnostic.empty() ? "face quality unavailable" : quality.diagnostic);
    if (quality.score < config_.minimumQuality)
    {
        auto result = failure(EnrollmentDecision::QualityRejected, "face quality is below the configured gate");
        result.quality = quality.score;
        return result;
    }

    auto presentation = presentation_.evaluate(alignment.alignedFace.view());
    if (presentation.state != PresentationState::Live)
    {
        auto result = failure(presentation.state == PresentationState::Spoof
                                  ? EnrollmentDecision::SpoofRejected
                                  : EnrollmentDecision::PresentationUnavailable,
                              presentation.diagnostic.empty() ? "live presentation was not established"
                                                              : presentation.diagnostic);
        result.quality = quality.score;
        result.presentation = std::move(presentation);
        return result;
    }

    auto embedding = embedding_.extract(alignment.alignedFace.view());
    if (!embedding.available || embedding.embedding.values.empty() ||
        embedding.embedding.modelId.empty() || embedding.embedding.modelId != embedding_.modelId())
    {
        auto result = failure(EnrollmentDecision::EmbeddingUnavailable,
                              embedding.diagnostic.empty() ? "face embedding unavailable"
                                                           : embedding.diagnostic);
        result.quality = quality.score;
        result.presentation = std::move(presentation);
        return result;
    }
    for (const float value : embedding.embedding.values)
        if (!std::isfinite(value))
            return failure(EnrollmentDecision::EmbeddingUnavailable, "face embedding contains non-finite values");

    EnrollmentEvaluation result;
    result.quality = quality.score;
    result.presentation = std::move(presentation);
    result.embedding = std::move(embedding.embedding);
    if (!config_.thresholdsApproved)
    {
        result.decision = EnrollmentDecision::ThresholdApprovalRequired;
        result.diagnostic = "diagnostic pipeline passed, but enrollment thresholds are not approved";
        return result;
    }
    result.decision = EnrollmentDecision::Accepted;
    result.diagnostic = "enrollment gates passed";
    return result;
}
} // namespace dms
