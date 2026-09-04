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
                           "--warmup=7", "--frames=42", "--output=result.json",
                           "--trace=frames.csv"};
    ok &= expect(parseBenchmarkOptions(7, valid, options, error), "valid options");
    ok &= expect(options.backend == BackendKind::MediaPipe, "backend");
    ok &= expect(options.warmupFrames == 7 && options.measuredFrames == 42,
                 "frame counts");
    ok &= expect(options.input == "face.jpg" && options.output == "result.json",
                 "paths");
    ok &= expect(options.trace == "frames.csv", "trace path");
    const char* camera[] = {"bench", "--camera=0", "--backend=yunet", "--frames=30"};
    ok &= expect(parseBenchmarkOptions(4, camera, options, error), "camera options");
    ok &= expect(options.cameraIndex && *options.cameraIndex == 0 && options.input.empty(),
                 "camera index");
    const char* inputAndCamera[] = {"bench", "--input=face.mp4", "--camera=0"};
    ok &= expect(!parseBenchmarkOptions(3, inputAndCamera, options, error),
                 "camera and recorded input are mutually exclusive");
    const char* badCamera[] = {"bench", "--camera=-1"};
    ok &= expect(!parseBenchmarkOptions(2, badCamera, options, error),
                 "negative camera rejected");
    const char* resources[] = {"bench", "--input=face.mp4",
                               "--resource-trace=resources.csv",
                               "--resource-sample-ms=125"};
    ok &= expect(parseBenchmarkOptions(4, resources, options, error),
                 "resource profiling options");
    ok &= expect(options.resourceTrace == "resources.csv" &&
                 options.resourceSampleMilliseconds == 125 && options.resourceProfile,
                 "resource profiling values");
    const char* presentation[] = {"bench", "--input=face.mp4",
                                  "--presentation-config=display.conf"};
    ok &= expect(parseBenchmarkOptions(3, presentation, options, error),
                 "presentation config option");
    ok &= expect(options.presentationConfig == "display.conf",
                 "presentation config path");
    const char* duplicatePresentation[] = {"bench", "--input=face.mp4",
        "--presentation-config=a.conf", "--presentation-config=b.conf"};
    ok &= expect(!parseBenchmarkOptions(4, duplicatePresentation, options, error),
                 "duplicate presentation config rejected");
    const char* crops[] = {"bench", "--input=face.mp4", "--eye-crops-dir=private",
                           "--eye-crop-every=9"};
    ok &= expect(parseBenchmarkOptions(4, crops, options, error), "crop options");
    ok &= expect(options.eyeCropsDirectory == "private" && options.eyeCropEvery == 9,
                 "crop option values");
    const char* demo[] = {"bench", "--input=face.mp4", "--sponsor-demo",
                          "--sponsor-demo-auto-exit"};
    ok &= expect(parseBenchmarkOptions(4, demo, options, error), "sponsor demo option");
    ok &= expect(options.sponsorDemo && options.sponsorDemoAutoExit,
                 "sponsor demo enabled");
    const char* autoExitOnly[] = {"bench", "--input=face.mp4", "--sponsor-demo-auto-exit"};
    ok &= expect(!parseBenchmarkOptions(3, autoExitOnly, options, error),
                 "auto exit requires demo");

    const char* pfld[] = {"bench", "--input=face.mp4", "--backend=pfld",
                          "--pfld-model=landmarks.onnx"};
    ok &= expect(parseBenchmarkOptions(4, pfld, options, error), "valid PFLD");
    ok &= expect(options.backend == BackendKind::Pfld &&
                 options.pfldModel == "landmarks.onnx", "PFLD options");
    const char* pfldMissing[] = {"bench", "--input=x", "--backend=pfld"};
    ok &= expect(!parseBenchmarkOptions(3, pfldMissing, options, error),
                 "PFLD model required");
    const char* strayModel[] = {"bench", "--input=x", "--pfld-model=x.onnx"};
    ok &= expect(!parseBenchmarkOptions(3, strayModel, options, error),
                 "PFLD model rejected for other backend");

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
