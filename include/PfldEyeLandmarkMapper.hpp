#pragma once

#include "EyeLandmarks.hpp"
#include "FaceBackend.hpp"

#include <vector>

bool mapPfldEyeLandmarks(
    const std::vector<FaceLandmark>& source,
    SemanticEyeLandmarks& result);
