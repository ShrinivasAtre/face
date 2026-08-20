#include "FaceMediaPipe.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdint>
#include <iostream>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: mediapipe_smoke <face_landmarker.task> <image>\n";
        return 2;
    }

    const char* model_path = argv[1];
    const char* image_path = argv[2];

    const uint32_t api_version = face_mp_api_version();
    std::cout << "FaceMediaPipe API version: " << api_version << "\n";
    if (api_version != 1)
    {
        std::cerr << "Unexpected FaceMediaPipe API version\n";
        return 3;
    }

    FaceMPHandle* handle = face_mp_create(model_path);
    if (handle == nullptr)
    {
        std::cerr << "face_mp_create() failed for model: " << model_path << "\n";
        return 4;
    }

    cv::Mat image = cv::imread(image_path, cv::IMREAD_COLOR);
    if (image.empty())
    {
        std::cerr << "Could not read image: " << image_path << "\n";
        face_mp_destroy(handle);
        return 5;
    }

    std::vector<FaceMPLandmark> landmarks(600);
    FaceMPResult result{};
    result.landmarks = landmarks.data();
    result.landmark_capacity = static_cast<int32_t>(landmarks.size());

    const int32_t ok = face_mp_process_bgr(
        handle,
        image.data,
        image.cols,
        image.rows,
        static_cast<int32_t>(image.step),
        &result);

    if (!ok)
    {
        std::cerr << "face_mp_process_bgr() failed: "
                  << face_mp_last_error(handle) << "\n";
        face_mp_destroy(handle);
        return 6;
    }

    std::cout << "Image: " << image.cols << "x" << image.rows << "\n";
    std::cout << "Detected: " << result.detected << "\n";
    std::cout << "Landmarks: " << result.landmark_count << "\n";

    if (result.detected)
    {
        std::cout << "Face bbox: x=" << result.face_x
                  << " y=" << result.face_y
                  << " w=" << result.face_width
                  << " h=" << result.face_height << "\n";

        const int preview_count = result.landmark_count < 5
            ? result.landmark_count
            : 5;
        for (int i = 0; i < preview_count; ++i)
        {
            const auto& p = landmarks[static_cast<size_t>(i)];
            std::cout << "landmark[" << i << "] = ("
                      << p.x << ", " << p.y << ", " << p.z << ")\n";
        }
    }

    face_mp_destroy(handle);

    if (!result.detected || result.landmark_count <= 0)
    {
        std::cerr << "No face landmarks detected. Use an image containing a clear face.\n";
        return 7;
    }

    std::cout << "MediaPipe smoke test PASSED\n";
    return 0;
}
