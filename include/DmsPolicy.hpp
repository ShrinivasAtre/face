#pragma once

#include "DmsObservation.hpp"
#include "DmsTemporalEvents.hpp"

namespace dms
{
struct OperationalPolicyProfile
{
    const char *name = "stage20-approved-2026-08-28";
    EyeCalibration eyeCalibration;
    EyeTemporalConfig eye;
    ObservationQualityGateConfig eyeQuality;
    YawnConfig yawn;
    HeadPoseConfig headPose;
    DistractionConfig distraction;
    PresenceConfig presence;
    MonitoringAvailabilityConfig availability;
    DrowsinessConfig drowsiness;
    MonotonicTime neutralCalibration = std::chrono::seconds(2);
    MonotonicTime recalibrateAfterAbsence = std::chrono::seconds(1);
    MonotonicTime recalibrateAfterInvalidGeometry = std::chrono::seconds(2);

    static OperationalPolicyProfile stage20Approved() noexcept;
    bool validate(std::string &error) const noexcept;
};
}
