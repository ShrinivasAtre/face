#include "DmsPresentationConfigLoader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}

bool write(const std::filesystem::path &path, const std::string &content)
{
    std::ofstream output(path, std::ios::binary);
    output << content;
    return output.good();
}
}

int main()
{
    using namespace dms;
    const auto path = std::filesystem::temp_directory_path() / "dms-presentation-loader-test.conf";
    std::string error;

    if (!check(write(path,
        "# deterministic test\n"
        "schema_version = 1\n"
        "display.show_landmarks=true\n"
        "processing_roi.enabled=true\n"
        "processing_roi.x=0.25\n"
        "processing_roi.width=0.50\n"
        "statistics.rolling_window_seconds=60\n"), "write valid fixture")) return 1;
    const auto valid = DmsPresentationConfigLoader::load(path, error);
    if (!check(valid.has_value(), "valid file loads") ||
        !check(valid->display.showLandmarks, "display selection loads") ||
        !check(valid->processingRoi.enabled && valid->processingRoi.x == 0.25,
               "ROI selection loads") ||
        !check(valid->statistics.rollingWindowSeconds == 60,
               "statistics selection loads")) return 1;

    if (!check(write(path, "display.enabled=true\n"), "write missing schema fixture") ||
        !check(!DmsPresentationConfigLoader::load(path, error) && error.find("schema_version") != std::string::npos,
               "missing schema is rejected")) return 1;
    if (!check(write(path, "schema_version=1\ndisplay.enabled=true\ndisplay.enabled=false\n"),
               "write duplicate fixture") ||
        !check(!DmsPresentationConfigLoader::load(path, error) && error.find("duplicate") != std::string::npos,
               "duplicate key is rejected")) return 1;
    if (!check(write(path, "schema_version=1\ndisplay.enabeld=true\n"), "write unknown fixture") ||
        !check(!DmsPresentationConfigLoader::load(path, error) && error.find("unknown") != std::string::npos,
               "unknown key is rejected")) return 1;
    if (!check(write(path, "schema_version=1\ndisplay.enabled=yes\n"), "write bad bool fixture") ||
        !check(!DmsPresentationConfigLoader::load(path, error) && error.find("invalid value") != std::string::npos,
               "noncanonical boolean is rejected")) return 1;
    if (!check(write(path, "schema_version=1\nprocessing_roi.width=2.0\n"), "write invalid ROI fixture") ||
        !check(!DmsPresentationConfigLoader::load(path, error) && error.find("within the frame") != std::string::npos,
               "semantic validation runs after parsing")) return 1;

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    if (!check(!DmsPresentationConfigLoader::load(path, error) && error.find("unable to open") != std::string::npos,
               "missing file is reported")) return 1;

    std::cout << "DMS presentation configuration loader tests PASSED\n";
    return 0;
}
