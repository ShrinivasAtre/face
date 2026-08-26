#pragma once
#include <string>

enum class BackendKind { YuNet, Pfld, MediaPipe };

struct BackendOptions
{
    BackendKind backend = BackendKind::YuNet;
    bool showHelp = false;
};

bool parseBackendOptions(int argc, const char* const argv[],
                         BackendOptions& options, std::string& error);
const char* backendName(BackendKind backend) noexcept;
std::string backendUsage(const char* programName);
