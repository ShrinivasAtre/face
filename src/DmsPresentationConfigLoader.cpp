#include "DmsPresentationConfigLoader.hpp"

#include <charconv>
#include <cctype>
#include <fstream>
#include <functional>
#include <limits>
#include <exception>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace dms
{
namespace
{
std::string trim(std::string_view input)
{
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.front()))) input.remove_prefix(1);
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.back()))) input.remove_suffix(1);
    return std::string(input);
}

bool parseBool(std::string_view text, bool &value)
{
    if (text == "true") { value = true; return true; }
    if (text == "false") { value = false; return true; }
    return false;
}

bool parseDisplayFocus(std::string_view text, DisplayFocus &value)
{
    if (text == "full") value = DisplayFocus::Full;
    else if (text == "face") value = DisplayFocus::Face;
    else if (text == "eyes") value = DisplayFocus::Eyes;
    else if (text == "mouth") value = DisplayFocus::Mouth;
    else return false;
    return true;
}

template <typename T>
bool parseNumber(std::string_view text, T &value)
{
    const char *begin = text.data();
    const char *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}
}

std::optional<DmsPresentationConfig> DmsPresentationConfigLoader::load(
    const std::filesystem::path &path, std::string &error) noexcept
{
    error.clear();
    try
    {
        std::ifstream input(path);
        if (!input)
        {
            error = "unable to open presentation configuration: " + path.string();
            return std::nullopt;
        }

        DmsPresentationConfig config;
        bool schemaSeen = false;
        std::unordered_set<std::string> seen;
        using Setter = std::function<bool(std::string_view)>;
        std::unordered_map<std::string, Setter> setters;
        const auto boolean = [&](const char *key, bool &target)
        {
            setters.emplace(key, [targetPointer = &target](std::string_view value)
            {
                return parseBool(value, *targetPointer);
            });
        };
        const auto number = [&](const char *key, double &target)
        {
            setters.emplace(key, [targetPointer = &target](std::string_view value)
            {
                return parseNumber(value, *targetPointer);
            });
        };

        setters.emplace("schema_version", [&](std::string_view value)
        {
            schemaSeen = true;
            return parseNumber(value, config.schemaVersion);
        });
        boolean("display.enabled", config.display.enabled);
        boolean("display.show_video", config.display.showVideo);
        setters.emplace("display.focus_region", [&](std::string_view value)
        {
            return parseDisplayFocus(value, config.display.focus);
        });
        boolean("display.show_face_box", config.display.showFaceBox);
        boolean("display.show_eye_boxes", config.display.showEyeBoxes);
        boolean("display.show_mouth_box", config.display.showMouthBox);
        boolean("display.show_landmarks", config.display.showLandmarks);
        boolean("display.show_quality", config.display.showQuality);
        boolean("display.show_calibration", config.display.showCalibration);
        boolean("display.show_presence", config.display.showPresence);
        boolean("display.show_eye_openness", config.display.showEyeOpenness);
        boolean("display.show_perclos", config.display.showPerclos);
        boolean("display.show_blink_counts", config.display.showBlinkCounts);
        boolean("display.show_yawn_counts", config.display.showYawnCounts);
        boolean("display.show_pose", config.display.showPose);
        boolean("display.show_gaze", config.display.showGaze);
        boolean("display.show_drowsiness", config.display.showDrowsiness);
        boolean("display.show_performance", config.display.showPerformance);
        boolean("processing_roi.enabled", config.processingRoi.enabled);
        number("processing_roi.x", config.processingRoi.x);
        number("processing_roi.y", config.processingRoi.y);
        number("processing_roi.width", config.processingRoi.width);
        number("processing_roi.height", config.processingRoi.height);
        number("processing_roi.minimum_face_coverage", config.processingRoi.minimumFaceCoverage);
        boolean("statistics.reset_on_confirmed_driver_change", config.statistics.resetOnConfirmedDriverChange);
        boolean("statistics.reset_on_session_start", config.statistics.resetOnSessionStart);
        setters.emplace("statistics.rolling_window_seconds", [&](std::string_view value)
        {
            return parseNumber(value, config.statistics.rollingWindowSeconds);
        });
        number("statistics.minimum_known_coverage", config.statistics.minimumKnownCoverage);
        boolean("statistics.cumulative_enabled", config.statistics.cumulativeEnabled);
        boolean("statistics.rolling_enabled", config.statistics.rollingEnabled);

        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(input, line))
        {
            ++lineNumber;
            const std::string cleaned = trim(line);
            if (cleaned.empty() || cleaned.front() == '#') continue;
            const auto separator = cleaned.find('=');
            if (separator == std::string::npos)
            {
                error = "configuration line " + std::to_string(lineNumber) + " requires key=value";
                return std::nullopt;
            }
            const std::string key = trim(std::string_view(cleaned).substr(0, separator));
            const std::string value = trim(std::string_view(cleaned).substr(separator + 1));
            const auto setter = setters.find(key);
            if (key.empty() || value.empty() || setter == setters.end())
            {
                error = "unknown or empty configuration entry on line " + std::to_string(lineNumber);
                return std::nullopt;
            }
            if (!seen.insert(key).second)
            {
                error = "duplicate configuration key on line " + std::to_string(lineNumber) + ": " + key;
                return std::nullopt;
            }
            if (!setter->second(value))
            {
                error = "invalid value for configuration key on line " + std::to_string(lineNumber) + ": " + key;
                return std::nullopt;
            }
        }
        if (!input.eof())
        {
            error = "unable to read presentation configuration: " + path.string();
            return std::nullopt;
        }
        if (!schemaSeen)
        {
            error = "presentation configuration requires schema_version";
            return std::nullopt;
        }
        if (!config.validate(error)) return std::nullopt;
        return config;
    }
    catch (const std::exception &exception)
    {
        error = "presentation configuration load failed: " + std::string(exception.what());
        return std::nullopt;
    }
    catch (...)
    {
        error = "presentation configuration load failed";
        return std::nullopt;
    }
}
}
