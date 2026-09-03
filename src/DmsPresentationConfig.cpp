#include "DmsPresentationConfig.hpp"

#include <algorithm>
#include <cmath>

namespace dms
{
bool ProcessingRoi::validate(std::string &error) const noexcept
{
    error.clear();
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(width) || !std::isfinite(height) ||
        !std::isfinite(minimumFaceCoverage))
    {
        error = "processing ROI values must be finite";
        return false;
    }
    if (x < 0.0 || y < 0.0 || width <= 0.0 || height <= 0.0 ||
        x + width > 1.0 || y + height > 1.0)
    {
        error = "processing ROI must be a positive normalized rectangle within the frame";
        return false;
    }
    if (minimumFaceCoverage < 0.0 || minimumFaceCoverage > 1.0)
    {
        error = "minimum face coverage must be within [0,1]";
        return false;
    }
    return true;
}

PixelRectangle ProcessingRoi::toPixels(int frameWidth, int frameHeight) const noexcept
{
    std::string error;
    if (frameWidth <= 0 || frameHeight <= 0 || !validate(error)) return {};
    if (!enabled) return {0, 0, frameWidth, frameHeight};

    const int left = std::clamp(static_cast<int>(std::floor(x * frameWidth)), 0, frameWidth);
    const int top = std::clamp(static_cast<int>(std::floor(y * frameHeight)), 0, frameHeight);
    const int right = std::clamp(static_cast<int>(std::ceil((x + width) * frameWidth)), left, frameWidth);
    const int bottom = std::clamp(static_cast<int>(std::ceil((y + height) * frameHeight)), top, frameHeight);
    return {left, top, right - left, bottom - top};
}

bool StatisticsSelection::validate(std::string &error) const noexcept
{
    error.clear();
    if (rollingWindowSeconds == 0)
    {
        error = "statistics rolling window must be greater than zero";
        return false;
    }
    if (!std::isfinite(minimumKnownCoverage) ||
        minimumKnownCoverage < 0.0 || minimumKnownCoverage > 1.0)
    {
        error = "statistics minimum known coverage must be within [0,1]";
        return false;
    }
    return true;
}

bool DmsPresentationConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (schemaVersion != currentSchemaVersion)
    {
        error = "unsupported presentation configuration schema version";
        return false;
    }
    return processingRoi.validate(error) && statistics.validate(error);
}
}
