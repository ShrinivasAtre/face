#include "AppPaths.hpp"
#include "BackendEyeMapper.hpp"
#include "BackendOptions.hpp"
#include "BlinkTracker.hpp"
#include "FaceBackend.hpp"
#include "YuNetLbfBackend.hpp"

#ifdef FACE_MEDIAPIPE_RUNTIME_ENABLED
#include "MediaPipeBackend.hpp"
#endif

#include <opencv2/opencv.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    BackendOptions options;
    std::string optionError;
    if (!parseBackendOptions(argc, argv, options, optionError))
    {
        std::cerr << "Error: " << optionError << '\n'
                  << backendUsage(argc > 0 ? argv[0] : nullptr) << '\n';
        return 2;
    }
    if (options.showHelp)
    {
        std::cout << backendUsage(argc > 0 ? argv[0] : nullptr) << '\n';
        return 0;
    }

#ifndef FACE_MEDIAPIPE_RUNTIME_ENABLED
    if (options.backend == BackendKind::MediaPipe)
    {
        std::cerr << "Error: MediaPipe backend is not available in this build. "
                  << "Configure with FACE_ENABLE_MEDIAPIPE_RUNTIME=ON.\n";
        return 2;
    }
#endif

    try
    {
        const std::filesystem::path executableDir = AppPaths::executableDirectory();
        const std::filesystem::path modelDir = executableDir / "models";
        const auto yunetModel = modelDir / "face_detection_yunet_2026may.onnx";
        const auto lbfModel = modelDir / "lbfmodel.yaml";

        std::cout << "Selected backend: " << backendName(options.backend) << '\n';

        cv::VideoCapture cap;
#ifdef _WIN32
        cap.open(0);
#else
        cap.open(0, cv::CAP_V4L2);
#endif
        if (!cap.isOpened())
        {
            std::cerr << "Error: Camera unavailable.\n";
            return -1;
        }
#ifndef _WIN32
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
#endif
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        cap.set(cv::CAP_PROP_FPS, 30);

        std::cout << "Camera backend: " << cap.getBackendName() << '\n'
                  << "Camera resolution: " << cap.get(cv::CAP_PROP_FRAME_WIDTH)
                  << " x " << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << '\n'
                  << "Camera FPS: " << cap.get(cv::CAP_PROP_FPS) << '\n';

        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
        {
            std::cerr << "ERROR: First camera frame is empty.\n";
            return -1;
        }

        std::unique_ptr<FaceBackend> backend;
        std::string backendModel;
        if (options.backend == BackendKind::YuNet)
        {
            backend = std::make_unique<YuNetLbfBackend>(lbfModel.string());
            backendModel = yunetModel.string();
        }
#ifdef FACE_MEDIAPIPE_RUNTIME_ENABLED
        else
        {
#ifdef _WIN32
            const auto libraryPath = executableDir / "FaceMediaPipe.dll";
#else
            const auto libraryPath = executableDir / "libFaceMediaPipe.so";
#endif
            backend = std::make_unique<MediaPipeBackend>(libraryPath);
            backendModel =
                (modelDir / "mediapipe" / "face_landmarker.task").string();
        }
#endif

        if (!backend || !backend->initialize(backendModel))
        {
            std::cerr << "Error: Failed to initialize selected backend.\n";
            return -1;
        }

        BlinkTracker blinkTracker(0.27);
        std::cout << "Backend initialized: " << backend->name() << '\n'
                  << "Memory-locked engine initialized. Ready.\n";

        using Clock = std::chrono::steady_clock;
        double captureTimeMs = 0.0;
        double backendTimeMs = 0.0;
        double totalTimeMs = 0.0;
        int performanceFrames = 0;
        auto performanceStart = Clock::now();

        while (true)
        {
            const auto frameStart = Clock::now();
            const auto captureStart = Clock::now();
            cap >> frame;
            const auto captureEnd = Clock::now();
            if (frame.empty())
            {
                std::cerr << "Warning: Empty frame.\n";
                continue;
            }
            captureTimeMs += std::chrono::duration<double, std::milli>(
                captureEnd - captureStart).count();

            const auto backendStart = Clock::now();
            FaceResult faceResult;
            const bool backendSuccess = backend->process(frame, faceResult);
            bool landmarkSuccess = false;
            if (backendSuccess && faceResult.detected)
            {
                SemanticEyeLandmarks eyeLandmarks;
                if (mapBackendEyeLandmarks(options.backend, faceResult, eyeLandmarks))
                {
                    landmarkSuccess = blinkTracker.process(frame, eyeLandmarks);
                }
                cv::rectangle(frame, faceResult.faceBox, cv::Scalar(255, 0, 0), 2);
            }
            const auto backendEnd = Clock::now();
            backendTimeMs += std::chrono::duration<double, std::milli>(
                backendEnd - backendStart).count();

            std::string text = "Blinks: " +
                std::to_string(blinkTracker.getBlinkCount());
            text += landmarkSuccess
                ? " | EAR: " + cv::format("%.2f", blinkTracker.getEAR())
                : " | EAR: RECOVERING";
            text += blinkTracker.isEyeClosed() ? " [CLOSED]" : " [OPEN]";
            cv::putText(
                frame, text, cv::Point(30, 50), cv::FONT_HERSHEY_SIMPLEX,
                0.8,
                blinkTracker.isEyeClosed()
                    ? cv::Scalar(0, 0, 255)
                    : cv::Scalar(0, 255, 0),
                2);

            const auto frameEnd = Clock::now();
            totalTimeMs += std::chrono::duration<double, std::milli>(
                frameEnd - frameStart).count();
            ++performanceFrames;

            const auto performanceNow = Clock::now();
            const double elapsedSeconds = std::chrono::duration<double>(
                performanceNow - performanceStart).count();
            if (elapsedSeconds >= 1.0)
            {
                std::cout << "\n----------------------------------------\n"
                          << "Performance (" << backend->name() << ")\n"
                          << "FPS          : " << performanceFrames / elapsedSeconds << '\n'
                          << "Capture      : " << captureTimeMs / performanceFrames << " ms\n"
                          << "Backend      : " << backendTimeMs / performanceFrames << " ms\n"
                          << "Total        : " << totalTimeMs / performanceFrames << " ms\n"
                          << "----------------------------------------\n" << std::flush;
                performanceStart = performanceNow;
                performanceFrames = 0;
                captureTimeMs = 0.0;
                backendTimeMs = 0.0;
                totalTimeMs = 0.0;
            }

            cv::imshow("Face Backend + Geometric EAR Blink Tracker", frame);
            const char key = static_cast<char>(cv::waitKey(1));
            if (key == 27 || key == 'q' || key == 'Q')
            {
                break;
            }
        }

        cap.release();
        cv::destroyAllWindows();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return -1;
    }
}
