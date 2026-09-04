#include "ProcessingRoiAdapter.hpp"

#include <cmath>

bool selectProcessingFrame(const cv::Mat &fullFrame,
                           const dms::ProcessingRoi &configuration,
                           cv::Mat &processingFrame,
                           ProcessingRoiContext &context,
                           std::string &error) noexcept
{
    processingFrame.release();
    context = {};
    error.clear();
    if (fullFrame.empty())
    {
        error = "processing ROI requires a non-empty frame";
        return false;
    }
    if (!configuration.validate(error)) return false;
    const dms::PixelRectangle pixels = configuration.toPixels(fullFrame.cols, fullFrame.rows);
    if (pixels.width <= 0 || pixels.height <= 0)
    {
        error = "processing ROI resolved to an empty pixel rectangle";
        return false;
    }
    context.fullFrameRectangle = {0, 0, fullFrame.cols, fullFrame.rows};
    context.processingRectangle = {pixels.x, pixels.y, pixels.width, pixels.height};
    processingFrame = fullFrame(context.processingRectangle);
    return true;
}

bool restoreProcessingResult(const ProcessingRoiContext &context,
                             const dms::ProcessingRoi &configuration,
                             FaceResult &result) noexcept
{
    if (!result.detected) return true;
    if (!configuration.enabled) return true;
    if (context.processingRectangle.width <= 0 || context.processingRectangle.height <= 0 ||
        result.faceBox.width <= 0 || result.faceBox.height <= 0)
    {
        result = {};
        return false;
    }

    const cv::Rect localBounds(0, 0, context.processingRectangle.width,
                              context.processingRectangle.height);
    const cv::Rect visible = result.faceBox & localBounds;
    const double detectedArea = static_cast<double>(result.faceBox.width) * result.faceBox.height;
    const double visibleArea = static_cast<double>(visible.width) * visible.height;
    const double coverage = detectedArea > 0.0 ? visibleArea / detectedArea : 0.0;
    if (!std::isfinite(coverage) || coverage < configuration.minimumFaceCoverage)
    {
        result = {};
        return false;
    }

    result.faceBox.x += context.processingRectangle.x;
    result.faceBox.y += context.processingRectangle.y;
    for (FaceLandmark &landmark : result.landmarks)
    {
        landmark.x += static_cast<float>(context.processingRectangle.x);
        landmark.y += static_cast<float>(context.processingRectangle.y);
    }
    return true;
}
