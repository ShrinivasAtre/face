#pragma once

#include "BackendOptions.hpp"
#include "FaceBackend.hpp"

struct SemanticGaze
{
    // Normalized approximately to [-1, 1]. Positive horizontal means the
    // driver's right; negative vertical means upward.
    float horizontal = 0.0F;
    float vertical = 0.0F;
    float interEyeAgreement = 0.0F;
};

// Gaze is available only when a provider exposes iris geometry. Providers
// without it return false rather than substituting head pose for eye gaze.
bool mapBackendGaze(BackendKind backend, const FaceResult &source, SemanticGaze &result) noexcept;
