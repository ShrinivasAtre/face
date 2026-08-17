#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <vector>

// Calculate the vertical and horizontal Eye Aspect Ratio (EAR)
double calculateEAR(const std::vector<cv::Point2f>& eye_points) {
    // eye_points contains 6 coordinates ordered around the eye perimeter clockwise
    // Distance between vertical eye landmarks
    double p2_p6 = cv::norm(eye_points[1] - eye_points[5]);
    double p3_p5 = cv::norm(eye_points[2] - eye_points[4]);

    // Distance between horizontal eye landmarks
    double p1_p4 = cv::norm(eye_points[0] - eye_points[3]);

    // Compute EAR formula
    return (p2_p6 + p3_p5) / (2.0 * p1_p4);
}

int main() {
    std::string yunet_model = "face_detection_yunet_2026may.onnx";
    std::string facemark_model = "lbfmodel.yaml";

    // Threshold configs for geometric detection
    const double EAR_THRESHOLD = 0.22; // Lower values mean narrower/closed eyes
    int blink_count = 0;
    bool was_blinking = false;

    // 1. Initialize Webcam
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Camera unavailable." << std::endl;
        return -1;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) return -1;

    // 2. Initialize YuNet Face Detector
    cv::Ptr<cv::FaceDetectorYN> detector = cv::FaceDetectorYN::create(
        yunet_model, "", frame.size(), 0.85f, 0.3f, 5000
    );

    // 3. Initialize Facemark LBF Detector
    cv::Ptr<cv::face::Facemark> facemark = cv::face::FacemarkLBF::create();
    facemark->loadModel(facemark_model);

    std::cout << "Geometric blink tracker started. Calibration complete." << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat faces;
        detector->detect(frame, faces);

        // Map YuNet results to OpenCV bounding boxes for Facemark input
        std::vector<cv::Rect> faces_boxes;
        for (int i = 0; i < faces.rows; i++) {
            int x = static_cast<int>(faces.at<float>(i, 0));
            int y = static_cast<int>(faces.at<float>(i, 1));
            int w = static_cast<int>(faces.at<float>(i, 2));
            int h = static_cast<int>(faces.at<float>(i, 3));
            faces_boxes.push_back(cv::Rect(x, y, w, h));
        }

        // Run landmark extraction if a face is present
        std::vector<std::vector<cv::Point2f>> landmarks;
        if (!faces_boxes.empty() && facemark->fit(frame, faces_boxes, landmarks)) {
            // Focus on the primary face array
            const auto& face_pts = landmarks[0];

            // Extract the specific indices representing both eyes in 68-point models
            // Right Eye indices: 36 to 41 | Left Eye indices: 42 to 47
            std::vector<cv::Point2f> right_eye(face_pts.begin() + 36, face_pts.begin() + 42);
            std::vector<cv::Point2f> left_eye(face_pts.begin() + 42, face_pts.begin() + 48);

            // Compute geometric ratios
            double right_ear = calculateEAR(right_eye);
            double left_ear = calculateEAR(left_eye);
            double average_ear = (right_ear + left_ear) / 2.0;

            // Draw eye silhouettes for reference visualization
            for (const auto& pt : right_eye) cv::circle(frame, pt, 2, cv::Scalar(0, 255, 255), -1);
            for (const auto& pt : left_eye) cv::circle(frame, pt, 2, cv::Scalar(0, 255, 255), -1);

            // Evaluate eye closure state independent of position changes
            bool currently_blinking = (average_ear < EAR_THRESHOLD);
            if (currently_blinking && !was_blinking) {
                blink_count++;
            }
            was_blinking = currently_blinking;

            // Print real-time updates onto UI
            std::string text = "Blinks: " + std::to_string(blink_count) + " | EAR: " + cv::format("%.2f", average_ear);
            if (currently_blinking) text += " [CLOSED]";
            cv::putText(frame, text, cv::Point(30, 50), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
        }

        cv::imshow("YuNet + Geometric EAR Blink Tracker", frame);

        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27 || key == 'q' || key == 'Q') break;
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
