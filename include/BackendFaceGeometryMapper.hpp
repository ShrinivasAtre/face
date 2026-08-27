#pragma once

#include "BackendOptions.hpp"
#include "FaceBackend.hpp"

#include <opencv2/core.hpp>

#include <optional>

// Provider-neutral semantic facial points used by downstream DMS metrics.
// Backend topology indices are confined to the mapper implementation.
struct SemanticFaceGeometry
{
    cv::Point2f noseTip;
    cv::Point2f chin;
    cv::Point2f rightEyeOuter;
    cv::Point2f leftEyeOuter;
    cv::Point2f rightMouthCorner;
    cv::Point2f leftMouthCorner;
    cv::Point2f upperInnerLip;
    cv::Point2f lowerInnerLip;
};

bool mapBackendFaceGeometry(BackendKind backend, const FaceResult &source, SemanticFaceGeometry &result);

// Returns a scale-independent value in [0, 1]. The metric uses inner-lip
// separation relative to mouth width and contains no provider topology.
std::optional<float> calculateMouthOpenness(const SemanticFaceGeometry &geometry) noexcept;
