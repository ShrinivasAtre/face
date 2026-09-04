#include "DriverIdentityMatcher.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dms
{
namespace
{
std::optional<float> cosineSimilarity(const FaceEmbedding &left, const FaceEmbedding &right) noexcept
{
    if (left.modelId.empty() || left.modelId != right.modelId || left.values.empty() ||
        left.values.size() != right.values.size())
        return std::nullopt;

    double dot = 0.0;
    double leftNorm = 0.0;
    double rightNorm = 0.0;
    for (std::size_t index = 0; index < left.values.size(); ++index)
    {
        const double a = left.values[index];
        const double b = right.values[index];
        if (!std::isfinite(a) || !std::isfinite(b))
            return std::nullopt;
        dot += a * b;
        leftNorm += a * a;
        rightNorm += b * b;
    }
    if (leftNorm <= 0.0 || rightNorm <= 0.0)
        return std::nullopt;
    return static_cast<float>(dot / std::sqrt(leftNorm * rightNorm));
}
} // namespace

bool IdentityMatcherConfig::validate(std::string &error) const noexcept
{
    if (!std::isfinite(matchThreshold) || matchThreshold < -1.0F || matchThreshold > 1.0F)
        error = "matchThreshold must be finite and within [-1,1]";
    else if (!std::isfinite(ambiguityMargin) || ambiguityMargin < 0.0F || ambiguityMargin > 2.0F)
        error = "ambiguityMargin must be finite and within [0,2]";
    else if (maximumProfiles == 0 || maximumProfiles > kMaximumDriverProfiles)
        error = "maximumProfiles must be within [1,50]";
    else
        error.clear();
    return error.empty();
}

DriverIdentityMatcher::DriverIdentityMatcher(IdentityMatcherConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}

IdentityDecision DriverIdentityMatcher::identify(const FaceEmbedding &query,
                                                 const std::vector<DriverTemplate> &gallery,
                                                 const PresentationResult &presentation) const noexcept
{
    IdentityDecision result;
    if (!valid_)
    {
        result.diagnostic = error_;
        return result;
    }
    if (presentation.state == PresentationState::Spoof)
    {
        result.state = IdentityState::SpoofRejected;
        result.diagnostic = "presentation attack rejected";
        return result;
    }
    if (presentation.state != PresentationState::Live)
    {
        result.diagnostic = "live presentation not established";
        return result;
    }
    if (gallery.empty())
    {
        result.state = IdentityState::Unknown;
        result.diagnostic = "gallery is empty";
        return result;
    }
    if (gallery.size() > config_.maximumProfiles)
    {
        result.diagnostic = "gallery exceeds configured profile limit";
        return result;
    }

    std::vector<IdentityCandidate> candidates;
    candidates.reserve(gallery.size());
    for (const auto &driver : gallery)
    {
        if (driver.driverId.empty() || driver.embeddings.empty())
            continue;
        float best = -std::numeric_limits<float>::infinity();
        for (const auto &enrolled : driver.embeddings)
        {
            const auto score = cosineSimilarity(query, enrolled);
            if (score)
                best = std::max(best, *score);
        }
        if (std::isfinite(best))
            candidates.push_back({driver.driverId, best});
    }
    if (candidates.empty())
    {
        result.diagnostic = "no compatible gallery templates";
        return result;
    }

    std::sort(candidates.begin(), candidates.end(), [](const auto &left, const auto &right) {
        return left.similarity > right.similarity;
    });
    result.similarity = candidates.front().similarity;
    if (result.similarity < config_.matchThreshold)
    {
        result.state = IdentityState::Unknown;
        return result;
    }
    if (candidates.size() > 1 && result.similarity - candidates[1].similarity < config_.ambiguityMargin)
    {
        result.state = IdentityState::Ambiguous;
        return result;
    }
    result.state = IdentityState::Candidate;
    result.driverId = candidates.front().driverId;
    return result;
}
} // namespace dms
