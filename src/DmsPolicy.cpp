#include "DmsPolicy.hpp"

namespace dms
{
OperationalPolicyProfile OperationalPolicyProfile::stage20Approved() noexcept
{
    OperationalPolicyProfile p;
    p.eyeQuality.policy = {0.5F, 0.5F, std::chrono::milliseconds(250)};
    p.eyeQuality.reacquisitionConfirmation = std::chrono::milliseconds(100);
    return p;
}

bool OperationalPolicyProfile::validate(std::string &error) const noexcept
{
    error.clear();
    if (!name || !*name) { error = "policy profile requires a name"; return false; }
    if (!eyeCalibration.validate(error) || !eye.validate(error) || !eyeQuality.validate(error) ||
        !yawn.validate(error) || !headPose.validate(error) || !distraction.validate(error) ||
        !presence.validate(error) || !availability.validate(error) || !drowsiness.validate(error)) return false;
    if (neutralCalibration <= MonotonicTime::zero() || recalibrateAfterAbsence <= MonotonicTime::zero() ||
        recalibrateAfterInvalidGeometry <= MonotonicTime::zero())
    {
        error = "invalid policy calibration durations";
        return false;
    }
    return true;
}
}
