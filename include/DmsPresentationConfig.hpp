#pragma once

#include <cstdint>
#include <string>

namespace dms
{
struct PixelRectangle
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct DisplaySelection
{
    bool enabled = true;
    bool showVideo = true;
    bool showFaceBox = true;
    bool showEyeBoxes = true;
    bool showMouthBox = true;
    bool showLandmarks = false;
    bool showQuality = true;
    bool showCalibration = true;
    bool showPresence = true;
    bool showEyeOpenness = true;
    bool showPerclos = true;
    bool showBlinkCounts = true;
    bool showYawnCounts = true;
    bool showPose = true;
    bool showGaze = true;
    bool showDrowsiness = true;
    bool showPerformance = true;
};

struct ProcessingRoi
{
    bool enabled = false;
    double x = 0.0;
    double y = 0.0;
    double width = 1.0;
    double height = 1.0;
    double minimumFaceCoverage = 0.80;

    bool validate(std::string &error) const noexcept;
    PixelRectangle toPixels(int frameWidth, int frameHeight) const noexcept;
};

struct StatisticsSelection
{
    bool resetOnConfirmedDriverChange = true;
    bool resetOnSessionStart = true;
    std::uint32_t rollingWindowSeconds = 300;
    double minimumKnownCoverage = 0.80;
    bool cumulativeEnabled = true;
    bool rollingEnabled = true;

    bool validate(std::string &error) const noexcept;
};

struct DmsPresentationConfig
{
    static constexpr std::uint32_t currentSchemaVersion = 1;

    std::uint32_t schemaVersion = currentSchemaVersion;
    DisplaySelection display;
    ProcessingRoi processingRoi;
    StatisticsSelection statistics;

    bool validate(std::string &error) const noexcept;
};
}
