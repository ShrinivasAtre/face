#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>

int main() {
    // 1. Define model paths and threshold settings
    std::string model_path = "face_detection_yunet_2026may.onnx";
    float score_threshold = 0.9f;
    float nms_threshold = 0.3f;
    int top_k = 5000;

    // 2. Initialize the default webcam (ID 0)
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the webcam." << std::endl;
        return -1;
    }

    // Set a predictable frame size for performance
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    // Read an initial frame to determine dimensions for YuNet
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        std::cerr << "Error: Captured empty frame." << std::endl;
        return -1;
    }

    // 3. Initialize YuNet with the capture dimensions
    cv::Ptr<cv::FaceDetectorYN> detector = cv::FaceDetectorYN::create(
        model_path, "", frame.size(), score_threshold, nms_threshold, top_k
    );

    std::cout << "Starting live feed. Press 'ESC' or 'q' on the video window to exit." << std::endl;

    // 4. Continuous processing loop
    while (true) {
        cap >> frame; // Capture the next live frame
        if (frame.empty()) break;

        // If your webcam dynamically changes resolution, update the detector size
        // detector->setInputSize(frame.size());

        // Run the YuNet detector on the live frame
        cv::Mat faces;
        detector->detect(frame, faces);

        // Draw bounding boxes and landmarks
        for (int i = 0; i < faces.rows; i++) {
            // Extract bounding box dimensions
            int x = static_cast<int>(faces.at<float>(i, 0));
            int y = static_cast<int>(faces.at<float>(i, 1));
            int w = static_cast<int>(faces.at<float>(i, 2));
            int h = static_cast<int>(faces.at<float>(i, 3));

            // Draw bounding box
            cv::rectangle(frame, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 0), 2);

            // Draw 5 facial landmark points
            for (int landmark = 0; landmark < 5; landmark++) {
                int lx = static_cast<int>(faces.at<float>(i, 4 + landmark * 2));
                int ly = static_cast<int>(faces.at<float>(i, 5 + landmark * 2));
                cv::circle(frame, cv::Point(lx, ly), 3, cv::Scalar(255, 0, 0), -1);
            }
        }

        // Show the processed live frame in a window
        cv::imshow("YuNet Live Face Detection", frame);

        // Break loop if user presses 'ESC' (27) or 'q'
        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    // Release hardware and close application windows cleanly
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
