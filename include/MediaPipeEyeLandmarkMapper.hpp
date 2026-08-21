#pragma once

#include "EyeLandmarks.hpp"
#include "FaceBackend.hpp"

#include <vector>

// Convert MediaPipe Face Landmarker pixel coordinates into the common
// subject-eye semantic ordering used by BlinkTracker. The source must use
// the 478-point Face Landmarker topology.
bool mapMediaPipeEyeLandmarks(
    const std::vector<FaceLandmark>& source,
    SemanticEyeLandmarks& result);
