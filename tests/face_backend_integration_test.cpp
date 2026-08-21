#include "BackendEyeMapper.hpp"
#include "BlinkTracker.hpp"
#include "FaceBackend.hpp"
#include "YuNetLbfBackend.hpp"

#ifdef FACE_MEDIAPIPE_RUNTIME_ENABLED
#include "MediaPipeBackend.hpp"
#endif

#include <opencv2/imgcodecs.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace
{
bool closeTo(double actual, double expected, double tolerance = 0.00001)
{
    return std::abs(actual - expected) <= tolerance;
}
}

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage: face_backend_integration_test "
                  << "<yunet|mediapipe> <image> <model-or-library> <lbf-or-task>\n";
        return 2;
    }

    const std::string selection = argv[1];
    const BackendKind kind = selection == "mediapipe"
        ? BackendKind::MediaPipe : BackendKind::YuNet;
    if (selection != "yunet" && selection != "mediapipe")
    {
        std::cerr << "Unsupported test backend: " << selection << '\n';
        return 2;
    }

    cv::Mat image = cv::imread(argv[2], cv::IMREAD_COLOR);
    if (image.empty())
    {
        std::cerr << "Failed to read image: " << argv[2] << '\n';
        return 1;
    }

    std::unique_ptr<FaceBackend> backend;
    std::string modelPath;
    if (kind == BackendKind::YuNet)
    {
        backend = std::make_unique<YuNetLbfBackend>(argv[4]);
        modelPath = argv[3];
    }
    else
    {
#ifdef FACE_MEDIAPIPE_RUNTIME_ENABLED
        backend = std::make_unique<MediaPipeBackend>(std::filesystem::path(argv[3]));
        modelPath = argv[4];
#else
        std::cerr << "MediaPipe integration is disabled.\n";
        return 2;
#endif
    }

    if (!backend->initialize(modelPath))
    {
        std::cerr << "Backend initialization failed.\n";
        return 1;
    }

    FaceResult result;
    if (!backend->process(image, result) || !result.detected ||
        !result.landmarksValid)
    {
        std::cerr << "Backend processing failed.\n";
        return 1;
    }

    const std::size_t expectedCount = kind == BackendKind::YuNet ? 68 : 478;
    const cv::Rect expectedBox = kind == BackendKind::YuNet
        ? cv::Rect(246, 437, 338, 452)
        : cv::Rect(243, 491, 350, 410);
    if (result.landmarks.size() != expectedCount || result.faceBox != expectedBox)
    {
        std::cerr << "Unexpected result: landmarks=" << result.landmarks.size()
                  << " bbox=" << result.faceBox << '\n';
        return 1;
    }

    SemanticEyeLandmarks eyes;
    BlinkTracker tracker(0.27);
    if (!mapBackendEyeLandmarks(kind, result, eyes) || !tracker.process(image, eyes))
    {
        std::cerr << "Shared eye/blink processing failed.\n";
        return 1;
    }
    if (kind == BackendKind::YuNet &&
        (!closeTo(tracker.getRightEAR(), 0.225832) ||
         !closeTo(tracker.getLeftEAR(), 0.238513)))
    {
        std::cerr << "YuNet/LBF EAR regression.\n";
        return 1;
    }

    FaceResult resetCheck = result;
    if (backend->process(cv::Mat(), resetCheck) || resetCheck.detected ||
        resetCheck.landmarksValid || !resetCheck.landmarks.empty())
    {
        std::cerr << "Invalid input did not reset the result.\n";
        return 1;
    }

    std::cout << "Backend: " << backend->name() << '\n'
              << "Detected: 1\n"
              << "Landmarks: " << result.landmarks.size() << '\n'
              << "Face bbox: x=" << result.faceBox.x
              << " y=" << result.faceBox.y
              << " w=" << result.faceBox.width
              << " h=" << result.faceBox.height << '\n'
              << "Right EAR: " << tracker.getRightEAR() << '\n'
              << "Left EAR: " << tracker.getLeftEAR() << '\n'
              << "Backend integration test PASSED\n";
    return 0;
}
