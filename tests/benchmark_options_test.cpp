#include "BenchmarkOptions.hpp"

#include <iostream>

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
    BenchmarkOptions options;
    std::string error;

    const char* valid[] = {"bench", "--input=face.jpg", "--backend=mediapipe",
                           "--warmup=7", "--frames=42", "--output=result.json"};
    ok &= expect(parseBenchmarkOptions(6, valid, options, error), "valid options");
    ok &= expect(options.backend == BackendKind::MediaPipe, "backend");
    ok &= expect(options.warmupFrames == 7 && options.measuredFrames == 42,
                 "frame counts");
    ok &= expect(options.input == "face.jpg" && options.output == "result.json",
                 "paths");

    const char* missing[] = {"bench", "--backend=yunet"};
    ok &= expect(!parseBenchmarkOptions(2, missing, options, error), "input required");
    const char* zero[] = {"bench", "--input=x", "--frames=0"};
    ok &= expect(!parseBenchmarkOptions(3, zero, options, error), "zero rejected");
    const char* duplicate[] = {"bench", "--input=x", "--input=y"};
    ok &= expect(!parseBenchmarkOptions(3, duplicate, options, error),
                 "duplicate rejected");
    const char* unknown[] = {"bench", "--input=x", "--other"};
    ok &= expect(!parseBenchmarkOptions(3, unknown, options, error),
                 "unknown rejected");

    if (!ok) return 1;
    std::cout << "Benchmark option tests PASSED\n";
    return 0;
}
