#pragma once

#include "DmsObservation.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dms
{
constexpr std::size_t kMaximumDriverProfiles = 50;

enum class PresentationState
{
    NotEvaluated,
    Live,
    Spoof,
    Indeterminate
};

enum class IdentityState
{
    Unavailable,
    Unknown,
    Ambiguous,
    Candidate,
    Matched,
    SpoofRejected
};

struct FaceImageView
{
    const std::uint8_t *bgr = nullptr;
    int width = 0;
    int height = 0;
    std::ptrdiff_t strideBytes = 0;
};

struct FaceEmbedding
{
    std::vector<float> values;
    std::string modelId;
};

struct DriverTemplate
{
    std::string driverId;
    std::vector<FaceEmbedding> embeddings;
};

struct EmbeddingResult
{
    bool available = false;
    float quality = 0.0F;
    FaceEmbedding embedding;
    std::string diagnostic;
};

struct PresentationResult
{
    PresentationState state = PresentationState::NotEvaluated;
    float score = 0.0F;
    std::string diagnostic;
};

struct IdentityCandidate
{
    std::string driverId;
    float similarity = 0.0F;
};

struct IdentityDecision
{
    IdentityState state = IdentityState::Unavailable;
    std::string driverId;
    float similarity = 0.0F;
    std::optional<float> calibratedConfidence;
    std::string diagnostic;
};

class FaceEmbeddingProvider
{
  public:
    virtual ~FaceEmbeddingProvider() = default;
    virtual const std::string &modelId() const noexcept = 0;
    virtual EmbeddingResult extract(const FaceImageView &alignedFace) = 0;
};

class PresentationAttackProvider
{
  public:
    virtual ~PresentationAttackProvider() = default;
    virtual const std::string &modelId() const noexcept = 0;
    virtual PresentationResult evaluate(const FaceImageView &face) = 0;
};

class DriverTemplateStore
{
  public:
    virtual ~DriverTemplateStore() = default;
    virtual std::vector<DriverTemplate> loadGallery() const = 0;
};
} // namespace dms
