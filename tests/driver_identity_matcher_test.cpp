#include "DriverIdentityMatcher.hpp"

#include <iostream>
#include <limits>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << '\n';
    return condition;
}

dms::FaceEmbedding embedding(std::initializer_list<float> values, const char *model = "test-v1")
{
    return {{values}, model};
}
} // namespace

int main()
{
    using namespace dms;
    const PresentationResult live{PresentationState::Live, 0.9F, {}};
    DriverIdentityMatcher matcher({0.8F, 0.05F, 50});
    const std::vector<DriverTemplate> gallery{{"driver-01", {embedding({1.0F, 0.0F})}},
                                              {"driver-02", {embedding({0.0F, 1.0F})}}};

    if (!check(matcher.valid(), "valid matcher") ||
        !check(matcher.identify(embedding({1.0F, 0.0F}), gallery, live).state == IdentityState::Candidate,
               "strong match remains a candidate until temporal confirmation") ||
        !check(matcher.identify(embedding({0.7F, 0.7F}), gallery, live).state == IdentityState::Unknown,
               "below-threshold query is unknown") ||
        !check(matcher.identify(embedding({1.0F, 1.0F}),
                                {{"a", {embedding({1.0F, 0.9F})}}, {"b", {embedding({0.9F, 1.0F})}}}, live)
                   .state == IdentityState::Ambiguous,
               "near-tied identities are ambiguous") ||
        !check(matcher.identify(embedding({1.0F, 0.0F}), gallery,
                                {PresentationState::Spoof, 0.99F, {}})
                   .state == IdentityState::SpoofRejected,
               "spoof fails closed") ||
        !check(matcher.identify(embedding({1.0F, 0.0F}), gallery,
                                {PresentationState::Indeterminate, 0.5F, {}})
                   .state == IdentityState::Unavailable,
               "indeterminate liveness cannot identify") ||
        !check(matcher.identify(embedding({1.0F, 0.0F}, "other-model"), gallery, live).state ==
                   IdentityState::Unavailable,
               "model-mismatched templates cannot compare"))
        return 1;

    DriverIdentityMatcher invalid({std::numeric_limits<float>::quiet_NaN(), 0.05F, 50});
    if (!check(!invalid.valid(), "non-finite threshold rejected"))
        return 1;
    std::cout << "driver identity matcher test PASSED\n";
    return 0;
}
