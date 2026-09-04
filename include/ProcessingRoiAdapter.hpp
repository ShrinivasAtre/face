#pragma once

#include "DmsPresentationConfig.hpp"
#include "FaceBackend.hpp"

#include <opencv2/core.hpp>

#include <string>

struct ProcessingRoiContext
{
    cv::Rect fullFrameRectangle;
    cv::Rect processingRectangle;
};

bool selectProcessingFrame(const cv::Mat &fullFrame,
                           const dms::ProcessingRoi &configuration,
                           cv::Mat &processingFrame,
                           ProcessingRoiContext &context,
                           std::string &error) noexcept;

// Returns true when an existing detection satisfies the ROI coverage rule.
// No detection is also a valid result and returns true. Rejected detections are
// cleared so downstream presence/quality contracts observe absence, not fatigue.
bool restoreProcessingResult(const ProcessingRoiContext &context,
                             const dms::ProcessingRoi &configuration,
                             FaceResult &result) noexcept;
