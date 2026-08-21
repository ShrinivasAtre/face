#include "BackendOptions.hpp"

#include <iostream>
#include <string>

namespace
{
bool expect(bool condition, const char* label)
{
    if (!condition) std::cerr << "FAILED: " << label << '\n';
    return condition;
}
}

int main()
{
    bool ok = true;
    BackendOptions options;
    std::string error;

    const char* defaultArgs[] = {"demo"};
    ok &= expect(parseBackendOptions(1, defaultArgs, options, error), "default parses");
    ok &= expect(options.backend == BackendKind::YuNet, "default is YuNet");

    const char* yunetArgs[] = {"demo", "--backend=yunet"};
    ok &= expect(parseBackendOptions(2, yunetArgs, options, error), "YuNet parses");
    ok &= expect(options.backend == BackendKind::YuNet, "explicit YuNet");

    const char* mediaPipeArgs[] = {"demo", "--backend=mediapipe"};
    ok &= expect(parseBackendOptions(2, mediaPipeArgs, options, error), "MediaPipe parses");
    ok &= expect(options.backend == BackendKind::MediaPipe, "explicit MediaPipe");

    const char* helpArgs[] = {"demo", "--help"};
    ok &= expect(parseBackendOptions(2, helpArgs, options, error), "help parses");
    ok &= expect(options.showHelp, "help selected");

    const char* badValue[] = {"demo", "--backend=other"};
    ok &= expect(!parseBackendOptions(2, badValue, options, error) && !error.empty(),
                 "unknown backend rejected");
    const char* malformed[] = {"demo", "--backend"};
    ok &= expect(!parseBackendOptions(2, malformed, options, error) && !error.empty(),
                 "malformed backend rejected");
    const char* duplicate[] = {"demo", "--backend=yunet", "--backend=mediapipe"};
    ok &= expect(!parseBackendOptions(3, duplicate, options, error) && !error.empty(),
                 "duplicate backend rejected");
    const char* unknown[] = {"demo", "--verbose"};
    ok &= expect(!parseBackendOptions(2, unknown, options, error) && !error.empty(),
                 "unknown option rejected");
    const char* helpCombined[] = {"demo", "--help", "--backend=yunet"};
    ok &= expect(!parseBackendOptions(3, helpCombined, options, error) && !error.empty(),
                 "combined help rejected");

    if (!ok) return 1;
    std::cout << "Backend option tests PASSED\n";
    return 0;
}
