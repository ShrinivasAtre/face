#include "AppPaths.hpp"
#include "FaceDetector.hpp"
#include "BlinkTracker.hpp"

#include <opencv2/opencv.hpp>

#include <chrono>
#include <iostream>
#include <string>

int main()
{
    try
    {
        // --------------------------------------------------
        // Model paths
        // --------------------------------------------------

        const auto modelDir =
            AppPaths::executableDirectory() / "models";

        const auto yunetModel =
            modelDir / "face_detection_yunet_2026may.onnx";

        const auto lbfModel =
            modelDir / "lbfmodel.yaml";

        std::cout
            << "YuNet model: "
            << yunetModel
            << std::endl;

        std::cout
            << "LBF model: "
            << lbfModel
            << std::endl;

        // --------------------------------------------------
        // Camera
        // --------------------------------------------------

        cv::VideoCapture cap;

#ifdef _WIN32

        cap.open(0);

#else

        cap.open(0, cv::CAP_V4L2);

#endif

        if (!cap.isOpened())
        {
            std::cerr
                << "Error: Camera unavailable."
                << std::endl;

            return -1;
        }

#ifndef _WIN32

        // Jetson / Linux:
        // Request MJPEG from the USB camera.

        cap.set(
            cv::CAP_PROP_FOURCC,
            cv::VideoWriter::fourcc(
                'M', 'J', 'P', 'G'
            )
        );

#endif

        // --------------------------------------------------
        // Camera configuration
        // --------------------------------------------------

        cap.set(
            cv::CAP_PROP_FRAME_WIDTH,
            640
        );

        cap.set(
            cv::CAP_PROP_FRAME_HEIGHT,
            480
        );

        cap.set(
            cv::CAP_PROP_FPS,
            30
        );

        std::cout
            << "Camera backend: "
            << cap.getBackendName()
            << std::endl;

        std::cout
            << "Camera resolution: "
            << cap.get(cv::CAP_PROP_FRAME_WIDTH)
            << " x "
            << cap.get(cv::CAP_PROP_FRAME_HEIGHT)
            << std::endl;

        std::cout
            << "Camera FPS: "
            << cap.get(cv::CAP_PROP_FPS)
            << std::endl;

        // --------------------------------------------------
        // Get first frame
        // --------------------------------------------------

        cv::Mat frame;

        cap >> frame;

        if (frame.empty())
        {
            std::cerr
                << "ERROR: First camera frame is empty."
                << std::endl;

            return -1;
        }

        std::cout
            << "First frame received: "
            << frame.cols
            << " x "
            << frame.rows
            << std::endl;

        // --------------------------------------------------
        // Face detector
        // --------------------------------------------------

        FaceDetector faceDetector(
            yunetModel.string(),
            frame.size(),
            0.60f,
            0.3f,
            5000
        );

        // --------------------------------------------------
        // Blink tracker
        // --------------------------------------------------

        BlinkTracker blinkTracker(
            lbfModel.string(),
            0.27
        );

        std::cout
            << "Memory-locked engine initialized. Ready."
            << std::endl;

        // --------------------------------------------------
        // Performance measurement
        // --------------------------------------------------

        using Clock = std::chrono::steady_clock;

        double captureTimeMs = 0.0;
        double yunetTimeMs = 0.0;
        double lbfTimeMs = 0.0;
        double totalTimeMs = 0.0;

        int performanceFrames = 0;

        auto performanceStart = Clock::now();

        // --------------------------------------------------
        // Main loop
        // --------------------------------------------------

        while (true)
        {
            auto frameStart = Clock::now();

            // --------------------------------------------------
            // Camera capture
            // --------------------------------------------------

            auto captureStart = Clock::now();

            cap >> frame;

            auto captureEnd = Clock::now();

            if (frame.empty())
            {
                std::cerr
                    << "Warning: Empty frame."
                    << std::endl;

                continue;
            }

            captureTimeMs +=
                std::chrono::duration<double, std::milli>(
                    captureEnd - captureStart
                ).count();

            // --------------------------------------------------
            // Face detection - YuNet
            // --------------------------------------------------

            cv::Rect faceBox;

            auto yunetStart = Clock::now();

            bool faceFound =
                faceDetector.detect(
                    frame,
                    faceBox
                );

            auto yunetEnd = Clock::now();

            yunetTimeMs +=
                std::chrono::duration<double, std::milli>(
                    yunetEnd - yunetStart
                ).count();

            // --------------------------------------------------
            // Blink / landmark processing - LBF
            // --------------------------------------------------

            bool landmarkSuccess = false;

            auto lbfStart = Clock::now();

            if (faceFound)
            {
                landmarkSuccess =
                    blinkTracker.process(
                        frame,
                        faceBox
                    );

                cv::rectangle(
                    frame,
                    faceBox,
                    cv::Scalar(255, 0, 0),
                    2
                );
            }

            auto lbfEnd = Clock::now();

            lbfTimeMs +=
                std::chrono::duration<double, std::milli>(
                    lbfEnd - lbfStart
                ).count();

            // --------------------------------------------------
            // Text overlay
            // --------------------------------------------------

            std::string text =
                "Blinks: " +
                std::to_string(
                    blinkTracker.getBlinkCount()
                );

            if (landmarkSuccess)
            {
                text +=
                    " | EAR: " +
                    cv::format(
                        "%.2f",
                        blinkTracker.getEAR()
                    );
            }
            else
            {
                text +=
                    " | EAR: RECOVERING";
            }

            if (blinkTracker.isEyeClosed())
            {
                text += " [CLOSED]";

                cv::putText(
                    frame,
                    text,
                    cv::Point(30, 50),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.8,
                    cv::Scalar(0, 0, 255),
                    2
                );
            }
            else
            {
                text += " [OPEN]";

                cv::putText(
                    frame,
                    text,
                    cv::Point(30, 50),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.8,
                    cv::Scalar(0, 255, 0),
                    2
                );
            }

            // --------------------------------------------------
            // Total frame processing time
            // --------------------------------------------------

            auto frameEnd = Clock::now();

            totalTimeMs +=
                std::chrono::duration<double, std::milli>(
                    frameEnd - frameStart
                ).count();

            performanceFrames++;

            // --------------------------------------------------
            // Performance reporting
            //
            // Print approximately once per second.
            // --------------------------------------------------

            auto performanceNow = Clock::now();

            double elapsedSeconds =
                std::chrono::duration<double>(
                    performanceNow - performanceStart
                ).count();

            if (elapsedSeconds >= 1.0)
            {
                double fps =
                    performanceFrames /
                    elapsedSeconds;

                double avgCapture =
                    captureTimeMs /
                    performanceFrames;

                double avgYuNet =
                    yunetTimeMs /
                    performanceFrames;

                double avgLBF =
                    lbfTimeMs /
                    performanceFrames;

                double avgTotal =
                    totalTimeMs /
                    performanceFrames;

                std::cout
                    << "\n"
                    << "----------------------------------------\n"
                    << "Performance\n"
                    << "FPS          : "
                    << fps
                    << "\n"
                    << "Capture      : "
                    << avgCapture
                    << " ms\n"
                    << "YuNet        : "
                    << avgYuNet
                    << " ms\n"
                    << "LBF          : "
                    << avgLBF
                    << " ms\n"
                    << "Total        : "
                    << avgTotal
                    << " ms\n"
                    << "----------------------------------------\n"
                    << std::flush;

                // Reset one-second measurement window.

                performanceStart =
                    performanceNow;

                performanceFrames = 0;

                captureTimeMs = 0.0;
                yunetTimeMs = 0.0;
                lbfTimeMs = 0.0;
                totalTimeMs = 0.0;
            }

            // --------------------------------------------------
            // Display
            // --------------------------------------------------

            cv::imshow(
                "YuNet + Geometric EAR Blink Tracker",
                frame
            );

            char key =
                static_cast<char>(
                    cv::waitKey(1)
                );

            if (
                key == 27 ||
                key == 'q' ||
                key == 'Q'
            )
            {
                break;
            }
        }

        // --------------------------------------------------
        // Cleanup
        // --------------------------------------------------

        cap.release();

        cv::destroyAllWindows();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Fatal error: "
            << e.what()
            << std::endl;

        return -1;
    }
}