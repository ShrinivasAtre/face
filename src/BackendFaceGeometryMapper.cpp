#include "BackendFaceGeometryMapper.hpp"

#include <algorithm>
#include <cmath>

namespace
{
cv::Point2f point(const FaceLandmark &source)
{
    return {source.x, source.y};
}

bool finite(const cv::Point2f &value)
{
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool finite(const SemanticFaceGeometry &value)
{
    return finite(value.noseTip) && finite(value.chin) && finite(value.rightEyeOuter) && finite(value.leftEyeOuter) &&
           finite(value.rightMouthCorner) && finite(value.leftMouthCorner) && finite(value.upperInnerLip) &&
           finite(value.lowerInnerLip);
}
} // namespace

bool mapBackendFaceGeometry(BackendKind backend, const FaceResult &source, SemanticFaceGeometry &result)
{
    result = {};
    if (!source.detected || !source.landmarksValid)
        return false;

    if (backend == BackendKind::MediaPipe)
    {
        if (source.landmarks.size() < 478)
            return false;
        // MediaPipe Face Landmarker topology. Indices stay in this adapter.
        result = {point(source.landmarks[1]),   point(source.landmarks[152]), point(source.landmarks[33]),
                  point(source.landmarks[263]), point(source.landmarks[61]),  point(source.landmarks[291]),
                  point(source.landmarks[13]),  point(source.landmarks[14])};
    }
    else
    {
        if (source.landmarks.size() < 68)
            return false;
        // LBF and the evaluated PFLD candidate both expose iBUG-68 topology.
        result = {point(source.landmarks[30]), point(source.landmarks[8]),  point(source.landmarks[36]),
                  point(source.landmarks[45]), point(source.landmarks[48]), point(source.landmarks[54]),
                  point(source.landmarks[62]), point(source.landmarks[66])};
    }
    if (!finite(result))
    {
        result = {};
        return false;
    }
    return true;
}

std::optional<float> calculateMouthOpenness(const SemanticFaceGeometry &geometry) noexcept
{
    if (!finite(geometry))
        return std::nullopt;
    const float width = static_cast<float>(cv::norm(geometry.leftMouthCorner - geometry.rightMouthCorner));
    const float gap = static_cast<float>(cv::norm(geometry.lowerInnerLip - geometry.upperInnerLip));
    if (!std::isfinite(width) || !std::isfinite(gap) || width <= 1.0F)
        return std::nullopt;
    return std::clamp(2.0F * gap / width, 0.0F, 1.0F);
}
