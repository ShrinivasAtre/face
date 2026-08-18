#include "BlinkTracker.hpp"

#include <iostream>
#include <stdexcept>


BlinkTracker::BlinkTracker(
    const std::string& landmarkModelPath,
    double eyeCloseThreshold)
    : eyeCloseThreshold_(eyeCloseThreshold),
      blinkCount_(0),
      isEyeClosed_(false),
      currentEAR_(0.0)
{
    std::cout
        << "Loading LBF landmark model: "
        << landmarkModelPath
        << std::endl;


    // --------------------------------------------------
    // Create LBF facemark detector
    // --------------------------------------------------

    facemark_ =
        cv::face::FacemarkLBF::create();


    if (facemark_.empty())
    {
        throw std::runtime_error(
            "Failed to create LBF Facemark"
        );
    }


    // --------------------------------------------------
    // Load model
    // --------------------------------------------------

    facemark_->loadModel(
        landmarkModelPath
    );


    std::cout
        << "LBF landmark detector initialized."
        << std::endl;
}


// ======================================================
// EAR calculation
// ======================================================

double BlinkTracker::calculateEAR(
    const std::vector<cv::Point2f>& eyePoints) const
{
    if (eyePoints.size() < 6)
    {
        return 0.0;
    }


    // Standard 6-point eye EAR calculation:
    //
    //        p2       p3
    //         ●-------●
    //       /           \
    //     p1               p4
    //       \             /
    //         ●---------●
    //        p6          p5
    //

    double p2_p6 =
        cv::norm(
            eyePoints.at(1) -
            eyePoints.at(5)
        );


    double p3_p5 =
        cv::norm(
            eyePoints.at(2) -
            eyePoints.at(4)
        );


    double p1_p4 =
        cv::norm(
            eyePoints.at(0) -
            eyePoints.at(3)
        );


    if (p1_p4 == 0.0)
    {
        return 0.0;
    }


    return
        (p2_p6 + p3_p5) /
        (2.0 * p1_p4);
}


// ======================================================
// Process one frame
// ======================================================

bool BlinkTracker::process(
    cv::Mat& frame,
    const cv::Rect& faceBox)
{
    if (frame.empty())
    {
        return false;
    }


    if (
        faceBox.width <= 0 ||
        faceBox.height <= 0
    )
    {
        return false;
    }


    // --------------------------------------------------
    // LBF expects a vector of face bounding boxes
    // --------------------------------------------------

    std::vector<cv::Rect> faceBoxes;

    faceBoxes.push_back(
        faceBox
    );


    // --------------------------------------------------
    // Landmark output
    // --------------------------------------------------

    std::vector<
        std::vector<cv::Point2f>
    > landmarks;


    bool success =
        facemark_->fit(
            frame,
            faceBoxes,
            landmarks
        );


    if (!success)
    {
        return false;
    }


    if (landmarks.empty())
    {
        return false;
    }


    if (landmarks.at(0).size() < 48)
    {
        return false;
    }


    const auto& facePoints =
        landmarks.at(0);


    // --------------------------------------------------
    // 68-point LBF landmark layout
    //
    // Right eye: 36 - 41
    // Left eye : 42 - 47
    // --------------------------------------------------

    std::vector<cv::Point2f> rightEye(
        facePoints.begin() + 36,
        facePoints.begin() + 42
    );


    std::vector<cv::Point2f> leftEye(
        facePoints.begin() + 42,
        facePoints.begin() + 48
    );


    // --------------------------------------------------
    // Calculate EAR
    // --------------------------------------------------

    double rightEAR =
        calculateEAR(
            rightEye
        );


    double leftEAR =
        calculateEAR(
            leftEye
        );


    currentEAR_ =
        (rightEAR + leftEAR) / 2.0;


    // --------------------------------------------------
    // Draw eye landmarks
    // --------------------------------------------------

    for (const auto& point : rightEye)
    {
        cv::circle(
            frame,
            point,
            2,
            cv::Scalar(
                0,
                255,
                255
            ),
            -1
        );
    }


    for (const auto& point : leftEye)
    {
        cv::circle(
            frame,
            point,
            2,
            cv::Scalar(
                0,
                255,
                255
            ),
            -1
        );
    }


    // --------------------------------------------------
    // Determine whether eyes are closed
    // --------------------------------------------------

    bool eyesClosed =
        rightEAR < eyeCloseThreshold_ ||
        leftEAR < eyeCloseThreshold_;


    // --------------------------------------------------
    // Blink state machine
    //
    // OPEN -> CLOSED
    //        blinkCount++
    //
    // CLOSED -> OPEN
    //        reset state
    // --------------------------------------------------

    if (eyesClosed)
    {
        if (!isEyeClosed_)
        {
            blinkCount_++;

            isEyeClosed_ = true;
        }
    }
    else
    {
        // Only reset after tracking proves that the
        // eyes are open again.

        if (
            currentEAR_ >=
            eyeCloseThreshold_
        )
        {
            isEyeClosed_ = false;
        }
    }


    return true;
}


// ======================================================
// Get blink count
// ======================================================

int BlinkTracker::getBlinkCount() const
{
    return blinkCount_;
}


// ======================================================
// Get current EAR
// ======================================================

double BlinkTracker::getEAR() const
{
    return currentEAR_;
}


// ======================================================
// Get eye state
// ======================================================

bool BlinkTracker::isEyeClosed() const
{
    return isEyeClosed_;
}