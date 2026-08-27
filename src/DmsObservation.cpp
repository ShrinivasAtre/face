#include "DmsObservation.hpp"

#include <cmath>

namespace dms
{
bool ObservationQualityGateConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (!std::isfinite(policy.minimumConfidence) || !std::isfinite(policy.minimumVisibility) ||
        policy.minimumConfidence < 0.0F || policy.minimumConfidence > 1.0F || policy.minimumVisibility < 0.0F ||
        policy.minimumVisibility > 1.0F || policy.maximumAge < MonotonicTime::zero() ||
        reacquisitionConfirmation < MonotonicTime::zero())
    {
        error = "quality gate thresholds and durations are invalid";
        return false;
    }
    return true;
}

ObservationUsability classifyObservation(const ObservationHeader &header, MonotonicTime now,
                                         const ObservationPolicy &policy) noexcept
{
    if (header.validity == SourceValidity::Missing)
    {
        return ObservationUsability::Missing;
    }
    if (header.validity == SourceValidity::Occluded)
    {
        return ObservationUsability::Occluded;
    }
    if (now < header.source.capturedAt)
    {
        return ObservationUsability::FutureTimestamp;
    }
    if (now - header.source.capturedAt > policy.maximumAge)
    {
        return ObservationUsability::Stale;
    }
    if (!std::isfinite(header.confidence) || !std::isfinite(header.visibility) ||
        header.confidence < policy.minimumConfidence || header.visibility < policy.minimumVisibility)
    {
        return ObservationUsability::LowConfidence;
    }
    return ObservationUsability::Usable;
}

ObservationQualityGate::ObservationQualityGate(ObservationQualityGateConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}

void ObservationQualityGate::reset() noexcept
{
    hasTimestamp_ = false;
    lastTimestamp_ = {};
    usableSince_.reset();
    usableConfirmed_ = false;
}

ObservationUsability ObservationQualityGate::update(const ObservationHeader &header, MonotonicTime now) noexcept
{
    if (!valid_)
        return ObservationUsability::LowConfidence;
    if (hasTimestamp_ && now <= lastTimestamp_)
        return ObservationUsability::FutureTimestamp;
    hasTimestamp_ = true;
    lastTimestamp_ = now;

    const ObservationUsability classified = classifyObservation(header, now, config_.policy);
    if (classified != ObservationUsability::Usable)
    {
        usableSince_.reset();
        usableConfirmed_ = false;
        return classified;
    }
    if (usableConfirmed_)
        return ObservationUsability::Usable;
    if (!usableSince_)
        usableSince_ = now;
    if (now - *usableSince_ >= config_.reacquisitionConfirmation)
    {
        usableConfirmed_ = true;
        return ObservationUsability::Usable;
    }
    return ObservationUsability::Recovering;
}
} // namespace dms
