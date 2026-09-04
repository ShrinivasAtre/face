#pragma once

#include "BackendFaceGeometryMapper.hpp"
#include "DmsPresentationConfig.hpp"
#include "EyeLandmarks.hpp"

#include <opencv2/core.hpp>

namespace dms
{
struct DisplayAoi
{
    cv::Rect sourceRectangle;
    bool available = false;
};

// Selects presentation pixels only. The returned view must never be passed to
// a detector or any semantic processing component.
DisplayAoi selectDisplayAoi(cv::Size frameSize, DisplayFocus focus,
                            bool faceDetected, const cv::Rect &faceBox,
                            const SemanticEyeLandmarks *eyes,
                            const SemanticFaceGeometry *faceGeometry) noexcept;
}
