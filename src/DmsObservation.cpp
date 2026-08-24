#include "DmsObservation.hpp"

#include <cmath>

namespace dms
{
ObservationUsability classifyObservation(
    const ObservationHeader& header,
    MonotonicTime now,
    const ObservationPolicy& policy) noexcept
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
    if (!std::isfinite(header.confidence) ||
        !std::isfinite(header.visibility) ||
        header.confidence < policy.minimumConfidence ||
        header.visibility < policy.minimumVisibility)
    {
        return ObservationUsability::LowConfidence;
    }
    return ObservationUsability::Usable;
}
}
