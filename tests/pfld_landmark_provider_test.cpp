#include "FaceDetector.hpp"
#include "PfldEyeLandmarkMapper.hpp"
#include "PfldLandmarkProvider.hpp"

#include <opencv2/imgcodecs.hpp>

#include <cmath>
#include <iostream>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr << "usage: test <yunet-model> <pfld-model> <image>\n";
        return 2;
    }

    cv::Mat image = cv::imread(argv[3]);
    if (image.empty())
        return 1;

    FaceDetector detector(argv[1], cv::Size(320, 320), 0.60f, 0.3f, 5000);
    cv::Rect faceBox;
    if (!detector.detect(image, faceBox))
        return 1;

    PfldLandmarkProvider provider;
    if (!provider.initialize(argv[2]))
        return 1;

    std::vector<FaceLandmark> landmarks;
    if (!provider.detect(image, faceBox, landmarks) ||
        landmarks.size() != PfldLandmarkProvider::landmarkCount())
        return 1;
    for (const FaceLandmark& landmark : landmarks)
    {
        if (!std::isfinite(landmark.x) || !std::isfinite(landmark.y))
            return 1;
    }

    SemanticEyeLandmarks eyes;
    return mapPfldEyeLandmarks(landmarks, eyes) ? 0 : 1;
}

