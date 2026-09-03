#include "DmsPresentationConfig.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    using namespace dms;
    std::string error;
    DmsPresentationConfig config;
    if (!check(config.validate(error), "default configuration is valid")) return 1;

    const auto fullFrame = config.processingRoi.toPixels(1920, 1080);
    if (!check(fullFrame.x == 0 && fullFrame.y == 0 &&
               fullFrame.width == 1920 && fullFrame.height == 1080,
               "disabled ROI selects the full frame")) return 1;

    config.processingRoi.enabled = true;
    config.processingRoi.x = 0.25;
    config.processingRoi.y = 0.10;
    config.processingRoi.width = 0.50;
    config.processingRoi.height = 0.80;
    const auto pixels = config.processingRoi.toPixels(640, 480);
    if (!check(pixels.x == 160 && pixels.y == 48 &&
               pixels.width == 320 && pixels.height == 384,
               "normalized ROI converts deterministically")) return 1;

    config.processingRoi.x = 0.75;
    config.processingRoi.width = 0.50;
    if (!check(!config.validate(error) && error.find("within the frame") != std::string::npos,
               "out-of-frame ROI is rejected")) return 1;

    config = {};
    config.processingRoi.width = 0.0;
    if (!check(!config.validate(error), "zero-width ROI is rejected even when disabled")) return 1;

    config = {};
    config.processingRoi.x = std::numeric_limits<double>::quiet_NaN();
    if (!check(!config.validate(error), "non-finite ROI is rejected")) return 1;

    config = {};
    config.statistics.rollingWindowSeconds = 0;
    if (!check(!config.validate(error), "zero statistics window is rejected")) return 1;

    config = {};
    config.statistics.minimumKnownCoverage = 1.01;
    if (!check(!config.validate(error), "invalid statistics coverage is rejected")) return 1;

    config = {};
    ++config.schemaVersion;
    if (!check(!config.validate(error) && error.find("schema") != std::string::npos,
               "unknown schema version is rejected")) return 1;

    std::cout << "DMS presentation configuration tests PASSED\n";
    return 0;
}
