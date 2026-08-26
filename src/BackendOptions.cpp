#include "BackendOptions.hpp"

#include <string>

bool parseBackendOptions(int argc, const char* const argv[],
                         BackendOptions& options, std::string& error)
{
    options = {};
    error.clear();
    bool backendSeen = false;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index] != nullptr ? argv[index] : "";
        if (argument == "--help" || argument == "-h")
        {
            if (argc != 2)
            {
                error = "--help cannot be combined with other arguments.";
                return false;
            }
            options.showHelp = true;
            return true;
        }

        constexpr const char* prefix = "--backend=";
        if (argument.rfind(prefix, 0) != 0)
        {
            error = "Unsupported argument: " + argument;
            return false;
        }
        if (backendSeen)
        {
            error = "--backend may be specified only once.";
            return false;
        }
        backendSeen = true;

        const std::string value = argument.substr(std::char_traits<char>::length(prefix));
        if (value == "yunet")
        {
            options.backend = BackendKind::YuNet;
        }
        else if (value == "mediapipe")
        {
            options.backend = BackendKind::MediaPipe;
        }
        else
        {
            error = "Unsupported backend: '" + value + "'.";
            return false;
        }
    }
    return true;
}

const char* backendName(BackendKind backend) noexcept
{
    switch (backend)
    {
    case BackendKind::Pfld: return "pfld";
    case BackendKind::MediaPipe: return "mediapipe";
    default: return "yunet";
    }
}

std::string backendUsage(const char* programName)
{
    const std::string program = programName != nullptr ? programName : "yunet_demo";
    return "Usage: " + program +
        " [--backend=yunet|--backend=mediapipe]\nDefault backend: yunet";
}
