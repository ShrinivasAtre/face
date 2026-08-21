#include "BlinkTracker.hpp"
#include "FaceDetector.hpp"
#include "LbfLandmarkDetector.hpp"
#include "LbfEyeLandmarkMapper.hpp"

#include <opencv2/imgcodecs.hpp>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr
            << "Usage: legacy_blink_integration_test "
            << "<yunet-model> <lbf-model> <image>"
            << std::endl;
        return 2;
    }

    const std::string yunetModel = argv[1];
    const std::string lbfModel = argv[2];
    const std::string imagePath = argv[3];

    cv::Mat frame = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (frame.empty())
    {
        std::cerr << "Failed to read image: " << imagePath << std::endl;
        return 1;
    }

    FaceDetector faceDetector(
        yunetModel,
        frame.size(),
        0.60f,
        0.3f,
        5000);

    cv::Rect faceBox;
    if (!faceDetector.detect(frame, faceBox))
    {
        std::cerr << "YuNet did not detect a face" << std::endl;
        return 1;
    }

    LbfLandmarkDetector landmarkDetector(lbfModel);
    std::vector<cv::Point2f> landmarks;
    if (!landmarkDetector.detect(frame, faceBox, landmarks))
    {
        std::cerr << "LBF did not produce landmarks" << std::endl;
        return 1;
    }

    BlinkTracker blinkTracker(0.27);
    SemanticEyeLandmarks eyeLandmarks;
    if (!mapLbfEyeLandmarks(landmarks, eyeLandmarks) ||
        !blinkTracker.process(frame, eyeLandmarks) ||
        !blinkTracker.isLandmarkValid())
    {
        std::cerr << "BlinkTracker rejected LBF landmarks" << std::endl;
        return 1;
    }

    std::cout
        << "Detected: 1\n"
        << "LBF landmarks: " << landmarks.size() << '\n'
        << "Face bbox: x=" << faceBox.x
        << " y=" << faceBox.y
        << " w=" << faceBox.width
        << " h=" << faceBox.height << '\n'
        << "Right EAR: " << blinkTracker.getRightEAR() << '\n'
        << "Left EAR: " << blinkTracker.getLeftEAR() << '\n'
        << "Legacy blink integration test PASSED"
        << std::endl;

    return 0;
}
