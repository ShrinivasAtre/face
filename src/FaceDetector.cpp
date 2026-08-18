#include "FaceDetector.hpp"

#include <stdexcept>
#include <iostream>


FaceDetector::FaceDetector(
    const std::string& modelPath,
    const cv::Size& inputSize,
    float scoreThreshold,
    float nmsThreshold,
    int topK)
{
    std::cout
        << "Loading YuNet model: "
        << modelPath
        << std::endl;

    detector_ =
        cv::FaceDetectorYN::create(
            modelPath,
            "",
            inputSize,
            scoreThreshold,
            nmsThreshold,
            topK
        );

    if (detector_.empty())
    {
        throw std::runtime_error(
            "Failed to create YuNet face detector"
        );
    }

    std::cout
        << "YuNet detector initialized."
        << std::endl;
}


bool FaceDetector::detect(
    const cv::Mat& frame,
    cv::Rect& faceBox)
{
    if (frame.empty())
    {
        return false;
    }

    // Make sure YuNet knows the current frame size.
    // This is useful if camera resolution changes.
    detector_->setInputSize(frame.size());

    cv::Mat faces;

    detector_->detect(
        frame,
        faces
    );

    // No face detected.
    if (faces.empty() || faces.rows <= 0)
    {
        return false;
    }

    // --------------------------------------------------
    // For now we use the first detected face.
    //
    // YuNet output format:
    //
    // [x, y, width, height,
    //  right_eye_x, right_eye_y,
    //  left_eye_x, left_eye_y,
    //  nose_x, nose_y,
    //  right_mouth_x, right_mouth_y,
    //  left_mouth_x, left_mouth_y,
    //  score]
    // --------------------------------------------------

    int x =
        static_cast<int>(
            faces.at<float>(0, 0)
        );

    int y =
        static_cast<int>(
            faces.at<float>(0, 1)
        );

    int width =
        static_cast<int>(
            faces.at<float>(0, 2)
        );

    int height =
        static_cast<int>(
            faces.at<float>(0, 3)
        );


    cv::Rect detectedBox(
        x,
        y,
        width,
        height
    );


    // --------------------------------------------------
    // Clip bounding box to image boundaries
    // --------------------------------------------------

    const cv::Rect imageBounds(
        0,
        0,
        frame.cols,
        frame.rows
    );

    detectedBox &= imageBounds;


    // Invalid bounding box
    if (
        detectedBox.width <= 0 ||
        detectedBox.height <= 0
    )
    {
        return false;
    }


    faceBox = detectedBox;

    return true;
}