#pragma once

#include "BackendOptions.hpp"
#include "EyeLandmarks.hpp"
#include "FaceBackend.hpp"

bool mapBackendEyeLandmarks(BackendKind backend, const FaceResult& source,
                            SemanticEyeLandmarks& result);
