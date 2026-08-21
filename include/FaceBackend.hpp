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
// Both runtime-selectable implementations normalize their face box and
// source-topology landmarks into this contract. Topology-specific eye
// mapping remains outside this result and outside BlinkTracker.
struct FaceResult
{
    bool detected = false;
    bool landmarksValid = false;

    cv::Rect faceBox;
    std::vector<FaceLandmark> landmarks;
};

// Backend-independent interface used by the runtime-selectable application.
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
