#pragma once

#include "DriverIdentity.hpp"

#include <string>

namespace dms
{
struct IdentityMatcherConfig
{
    float matchThreshold = 0.0F;
    float ambiguityMargin = 0.0F;
    std::size_t maximumProfiles = kMaximumDriverProfiles;

    bool validate(std::string &error) const noexcept;
};

class DriverIdentityMatcher
{
  public:
    explicit DriverIdentityMatcher(IdentityMatcherConfig config);

    bool valid() const noexcept { return valid_; }
    const std::string &error() const noexcept { return error_; }
    IdentityDecision identify(const FaceEmbedding &query, const std::vector<DriverTemplate> &gallery,
                              const PresentationResult &presentation) const noexcept;

  private:
    IdentityMatcherConfig config_;
    bool valid_ = false;
    std::string error_;
};
} // namespace dms
