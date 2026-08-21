#include "BackendEyeMapper.hpp"

#include "LbfEyeLandmarkMapper.hpp"
#include "MediaPipeEyeLandmarkMapper.hpp"

#include <opencv2/core.hpp>
#include <vector>

bool mapBackendEyeLandmarks(BackendKind backend, const FaceResult& source,
                            SemanticEyeLandmarks& result)
{
    result = {};
    if (!source.detected || !source.landmarksValid)
    {
        return false;
    }
    if (backend == BackendKind::MediaPipe)
    {
        return mapMediaPipeEyeLandmarks(source.landmarks, result);
    }

    std::vector<cv::Point2f> landmarks;
    landmarks.reserve(source.landmarks.size());
    for (const FaceLandmark& landmark : source.landmarks)
    {
        landmarks.emplace_back(landmark.x, landmark.y);
    }
    return mapLbfEyeLandmarks(landmarks, result);
}
