#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

// Backend-independent facial landmark representation.
// Coordinates are expressed in image pixels. The z coordinate is optional
// and is currently used only by backends that provide depth information.
struct FaceLandmark
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Common result produced by a face-processing backend.
//
// The initial YuNet/LBF implementation currently exposes the face box and
// landmarks through two existing classes (FaceDetector and BlinkTracker).
// This structure establishes the common contract without changing that
// existing implementation yet. Wiring the existing classes into this
// contract is intentionally deferred until the landmark/blink separation
// stage of the MediaPipe integration.
struct FaceResult
{
    bool detected = false;
    bool landmarksValid = false;

    cv::Rect faceBox;
    std::vector<FaceLandmark> landmarks;
};

// Backend-independent interface used by the application.
//
// Implementations must not expose backend-specific types (for example,
// MediaPipe classes) through this interface.
class FaceBackend
{
public:
    virtual ~FaceBackend() = default;

    virtual bool initialize(
        const std::string& modelPath) = 0;

    virtual bool process(
        const cv::Mat& frame,
        FaceResult& result) = 0;

    virtual const char* name() const = 0;
};
