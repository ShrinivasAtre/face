#pragma once

#include "DmsObservation.hpp"
#include "EyeLandmarks.hpp"

#include <opencv2/core.hpp>

#include <string>

struct EyeQualityConfig
{
    float roiExpansion = 0.45F;
    float minimumVisibleFraction = 0.75F;
    float darkLimit = 15.0F;
    float brightLimit = 240.0F;
    float minimumContrast = 8.0F;
    float minimumLaplacianVariance = 4.0F;
    bool validate(std::string &error) const noexcept;
};

struct EyeQualityResult
{
    dms::SourceValidity validity = dms::SourceValidity::Missing;
    float confidence = 0.0F;
    float visibility = 0.0F;
    float meanIntensity = 0.0F;
    float contrast = 0.0F;
    float laplacianVariance = 0.0F;
};

// Assesses the actual eye image regions after topology has been mapped away.
// Partial/out-of-frame eye regions are occluded; exposure, contrast, or blur
// reduce confidence. It deliberately does not claim that a hand/object covering
// an in-frame eye can be classified without a trained ROI model.
class EyeQualityAssessor
{
  public:
    explicit EyeQualityAssessor(EyeQualityConfig config = {});
    bool valid() const noexcept
    {
        return valid_;
    }
    const std::string &error() const noexcept
    {
        return error_;
    }
    EyeQualityResult assess(const cv::Mat &frame, const SemanticEyeLandmarks &eyes) const noexcept;

  private:
    EyeQualityConfig config_;
    bool valid_ = false;
    std::string error_;
};
