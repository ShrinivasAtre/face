#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <vector>
#include <numeric>

// Helper structure to track eye blink state history
struct EyeState {
    bool is_closed = false;
    std::vector<double> variance_history;
    const size_t max_history = 100; // Rolling window for dynamic baseline calibration
};

// Function to check if an eye region is closed based on variance drop
bool checkEyeClosed(const cv::Mat& frame, int eye_x, int eye_y, int face_w, EyeState& state) {
    // Define a small bounding box centered around the eye point
    int crop_sz = std::max(4, face_w / 14); 
    cv::Rect eye_roi(eye_x - crop_sz, eye_y - crop_sz, crop_sz * 2, crop_sz * 2);

    // Keep ROI within frame boundaries
    eye_roi &= cv::Rect(0, 0, frame.cols, frame.rows);
    if (eye_roi.area() <= 0) return false;

    // Convert eye region to grayscale
    cv::Mat gray_eye;
    cv::cvtColor(frame(eye_roi), gray_eye, cv::COLOR_BGR2GRAY);

    // Calculate the standard deviation (contrast/texture intensity) of the eye patch
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray_eye, mean, stddev);
    double current_variance = stddev[0] * stddev[0];

    // Maintain a rolling baseline of the user's open-eye variance
    if (state.variance_history.size() < state.max_history) {
        state.variance_history.push_back(current_variance);
        return false;
    }

    // Calculate standard open-eye baseline from history
    double sum = std::accumulate(state.variance_history.begin(), state.variance_history.end(), 0.0);
    double average_variance = sum / state.variance_history.size();

    // A sharp drop in variance indicates the pupil is covered by smooth eyelid skin
    // If current contrast is less than 55% of baseline, the eye is closed
    bool closed = (current_variance < (average_variance * 0.55));

    // Only update history when eye is open to avoid corrupting the baseline calibration
    if (!closed) {
        state.variance_history.erase(state.variance_history.begin());
        state.variance_history.push_back(current_variance);
    }

    return closed;
}

int main() {
    std::string model_path = "face_detection_yunet_2026may.onnx";
    float score_threshold = 0.85f; // Lowered slightly for more stable tracking during tilts
    float nms_threshold = 0.3f;
    int top_k = 5000;

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Webcam not accessible." << std::endl;
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) return -1;

    cv::Ptr<cv::FaceDetectorYN> detector = cv::FaceDetectorYN::create(
        model_path, "", frame.size(), score_threshold, nms_threshold, top_k
    );

    // Tracking states for single face blink logic
    EyeState left_eye_state;
    EyeState right_eye_state;
    int blink_count = 0;
    bool was_blinking = false;

    std::cout << "Calibrating baseline... Keep eyes open and look at camera for 3 seconds." << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat faces;
        detector->detect(frame, faces);

        if (faces.rows > 0) {
            // Process the primary face (row 0)
            int x = static_cast<int>(faces.at<float>(0, 0));
            int y = static_cast<int>(faces.at<float>(0, 1));
            int w = static_cast<int>(faces.at<float>(0, 2));
            int h = static_cast<int>(faces.at<float>(0, 3));

            // Bounding box draw
            cv::rectangle(frame, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 0), 2);

            // Extract eye coordinates from the landmark array
            int right_eye_x = static_cast<int>(faces.at<float>(0, 4));
            int right_eye_y = static_cast<int>(faces.at<float>(0, 5));
            int left_eye_x  = static_cast<int>(faces.at<float>(0, 6));
            int left_eye_y  = static_cast<int>(faces.at<float>(0, 7));

            // Analyze both eye states
            bool right_closed = checkEyeClosed(frame, right_eye_x, right_eye_y, w, right_eye_state);
            bool left_closed  = checkEyeClosed(frame, left_eye_x, left_eye_y, w, left_eye_state);

            // Highlight eye states visually
            cv::circle(frame, cv::Point(right_eye_x, right_eye_y), 4, right_closed ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0), -1);
            cv::circle(frame, cv::Point(left_eye_x, left_eye_y), 4, left_closed ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0), -1);

            // Blink counting condition (Both eyes must close together)
            bool currently_blinking = (right_closed && left_closed);
            if (currently_blinking && !was_blinking) {
                blink_count++;
            }
            was_blinking = currently_blinking;

            // Overlay metrics text onto the display
            std::string status = "Blinks: " + std::to_string(blink_count);
            if (currently_blinking) status += " [BLINKING]";
            cv::putText(frame, status, cv::Point(30, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 255), 2);
        }

        cv::imshow("YuNet Eye Blink Tracker", frame);

        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27 || key == 'q' || key == 'Q') break;
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
