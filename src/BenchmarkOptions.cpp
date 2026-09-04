#include "BenchmarkOptions.hpp"

#include <charconv>
#include <limits>
#include <string_view>

namespace
{
bool parseCount(std::string_view text, std::size_t& value)
{
    unsigned long long parsed = 0;
    const auto result = std::from_chars(
        text.data(), text.data() + text.size(), parsed);
    if (text.empty() || result.ec != std::errc{} ||
        result.ptr != text.data() + text.size() || parsed == 0 ||
        parsed > std::numeric_limits<std::size_t>::max())
    {
        return false;
    }
    value = static_cast<std::size_t>(parsed);
    return true;
}
}

bool parseBenchmarkOptions(int argc, const char* const argv[],
                           BenchmarkOptions& options, std::string& error)
{
    options = {};
    error.clear();
    bool backendSeen = false;
    bool inputSeen = false;
    bool outputSeen = false;
    bool traceSeen = false;
    bool resourceTraceSeen = false;
    bool presentationConfigSeen = false;
    bool eyeCropsSeen = false;
    bool pfldModelSeen = false;
    bool warmupSeen = false;
    bool framesSeen = false;
    bool eyeCropEverySeen = false;
    bool resourceSampleSeen = false;
    bool sponsorDemoSeen = false;
    bool sponsorDemoAutoExitSeen = false;
    bool resourceProfileSeen = false;

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
        if (argument == "--sponsor-demo")
        {
            if (sponsorDemoSeen)
            {
                error = "--sponsor-demo may be specified only once.";
                return false;
            }
            sponsorDemoSeen = true;
            options.sponsorDemo = true;
            continue;
        }
        if (argument == "--sponsor-demo-auto-exit")
        {
            if (sponsorDemoAutoExitSeen)
            {
                error = "--sponsor-demo-auto-exit may be specified only once.";
                return false;
            }
            sponsorDemoAutoExitSeen = true;
            options.sponsorDemoAutoExit = true;
            continue;
        }
        if (argument == "--resource-profile")
        {
            if (resourceProfileSeen)
            {
                error = "--resource-profile may be specified only once.";
                return false;
            }
            resourceProfileSeen = true;
            options.resourceProfile = true;
            continue;
        }

        const auto parsePath = [&](const char* prefix, bool& seen,
                                   std::filesystem::path& destination) -> bool
        {
            const std::string_view prefixView(prefix);
            if (argument.rfind(prefix, 0) != 0) return false;
            if (seen)
            {
                error = std::string(prefixView.substr(0, prefixView.size() - 1)) +
                    " may be specified only once.";
                return true;
            }
            seen = true;
            destination = argument.substr(prefixView.size());
            if (destination.empty())
            {
                error = std::string(prefixView.substr(0, prefixView.size() - 1)) +
                    " requires a path.";
            }
            return true;
        };

        if (parsePath("--input=", inputSeen, options.input) ||
            parsePath("--output=", outputSeen, options.output) ||
            parsePath("--trace=", traceSeen, options.trace) ||
            parsePath("--resource-trace=", resourceTraceSeen, options.resourceTrace) ||
            parsePath("--presentation-config=", presentationConfigSeen, options.presentationConfig) ||
            parsePath("--eye-crops-dir=", eyeCropsSeen, options.eyeCropsDirectory) ||
            parsePath("--pfld-model=", pfldModelSeen, options.pfldModel))
        {
            if (!error.empty()) return false;
            continue;
        }
        if (argument.rfind("--backend=", 0) == 0)
        {
            if (backendSeen)
            {
                error = "--backend may be specified only once.";
                return false;
            }
            backendSeen = true;
            const std::string value = argument.substr(10);
            if (value == "yunet") options.backend = BackendKind::YuNet;
            else if (value == "pfld") options.backend = BackendKind::Pfld;
            else if (value == "mediapipe") options.backend = BackendKind::MediaPipe;
            else
            {
                error = "Unsupported backend: '" + value + "'.";
                return false;
            }
            continue;
        }

        const auto parseCountOption = [&](const char* prefix, bool& seen,
                                          std::size_t& destination) -> bool
        {
            const std::string_view prefixView(prefix);
            if (argument.rfind(prefix, 0) != 0) return false;
            if (seen)
            {
                error = std::string(prefixView.substr(0, prefixView.size() - 1)) +
                    " may be specified only once.";
                return true;
            }
            seen = true;
            if (!parseCount(std::string_view(argument).substr(prefixView.size()),
                            destination))
            {
                error = std::string(prefixView.substr(0, prefixView.size() - 1)) +
                    " requires a positive integer.";
            }
            return true;
        };
        if (parseCountOption("--warmup=", warmupSeen, options.warmupFrames) ||
            parseCountOption("--frames=", framesSeen, options.measuredFrames) ||
            parseCountOption("--eye-crop-every=", eyeCropEverySeen, options.eyeCropEvery))
        {
            if (!error.empty()) return false;
            continue;
        }
        if (parseCountOption("--resource-sample-ms=", resourceSampleSeen,
                             options.resourceSampleMilliseconds))
        {
            if (!error.empty()) return false;
            continue;
        }

        error = "Unsupported argument: " + argument;
        return false;
    }

    if (!inputSeen)
    {
        error = "--input is required.";
        return false;
    }
    if (resourceTraceSeen) options.resourceProfile = true;
    if (options.sponsorDemoAutoExit && !options.sponsorDemo)
    {
        error = "--sponsor-demo-auto-exit requires --sponsor-demo.";
        return false;
    }
    if (options.backend == BackendKind::Pfld && !pfldModelSeen)
    {
        error = "--pfld-model is required for the PFLD backend.";
        return false;
    }
    if (options.backend != BackendKind::Pfld && pfldModelSeen)
    {
        error = "--pfld-model is valid only with --backend=pfld.";
        return false;
    }
    return true;
}

std::string benchmarkUsage(const char* programName)
{
    const std::string program = programName != nullptr ? programName : "face_benchmark";
    return "Usage: " + program +
        " --input=<image-or-video> [--backend=yunet|pfld|mediapipe]"
        " [--pfld-model=<landmarks_68_pfld.onnx>]"
        " [--warmup=N] [--frames=N] [--output=results.json]"
        " [--trace=frames.csv] [--eye-crops-dir=directory]"
        " [--resource-profile] [--resource-trace=resources.csv] [--resource-sample-ms=N]"
        " [--presentation-config=dms-presentation.conf]"
        " [--eye-crop-every=N] [--sponsor-demo] [--sponsor-demo-auto-exit]";
}
