#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <vector>
#include "AppPaths.hpp"


// Reliable EAR calculator using explicit .at() mapping
double calculateEAR(const std::vector<cv::Point2f>& eye_points) {
    if (eye_points.size() < 6) return 0.0;
    double p2_p6 = cv::norm(eye_points.at(1) - eye_points.at(5));
    double p3_p5 = cv::norm(eye_points.at(2) - eye_points.at(4));
    double p1_p4 = cv::norm(eye_points.at(0) - eye_points.at(3));
    if (p1_p4 == 0.0) return 0.0;
    return (p2_p6 + p3_p5) / (2.0 * p1_p4);
}

int main() {
	
	const auto model_dir =
		AppPaths::executableDirectory() / "models";

	const auto yunet_model =
		model_dir / "face_detection_yunet_2026may.onnx";

	const auto facemark_model =
		model_dir / "lbfmodel.yaml";

    // Threshold optimized for your 0.34 open eye baseline
    const double EAR_CLOSE_THRESHOLD = 0.27;

    int blink_count = 0;
    bool is_eye_closed = false;

    // Persistent Bounding Box Memory to survive face tracking drops
    cv::Rect last_valid_face_box(0, 0, 0, 0);
    int face_lost_frame_counter = 0;
    const int MAX_FACE_LOST_GRACE_PERIOD = 15; // ~0.5 seconds at 30fps

	cv::VideoCapture cap;

	#ifdef _WIN32
		cap.open(0);
	#else
		cap.open(0, cv::CAP_V4L2);
	#endif

	if (!cap.isOpened()) {
		std::cerr << "Error: Camera unavailable." << std::endl;
		return -1;
	}

	#ifndef _WIN32
		cap.set(cv::CAP_PROP_FOURCC,
				cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
	#endif

	cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
	cap.set(cv::CAP_PROP_FPS, 30);

	std::cout << "Camera resolution: "
			  << cap.get(cv::CAP_PROP_FRAME_WIDTH)
			  << " x "
			  << cap.get(cv::CAP_PROP_FRAME_HEIGHT)
			  << std::endl;

	std::cout << "Camera FPS: "
			  << cap.get(cv::CAP_PROP_FPS)
			  << std::endl;

	if (!cap.isOpened()) {
		std::cerr << "Error: Camera unavailable." << std::endl;
		return -1;
	}

	cap.set(cv::CAP_PROP_FOURCC,
			cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));

	cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
	cap.set(cv::CAP_PROP_FPS, 30);

	std::cout << "Camera backend: "
			  << cap.getBackendName() << std::endl;

	std::cout << "Camera resolution: "
			  << cap.get(cv::CAP_PROP_FRAME_WIDTH)
			  << " x "
			  << cap.get(cv::CAP_PROP_FRAME_HEIGHT)
			  << std::endl;

	std::cout << "Camera FPS: "
			  << cap.get(cv::CAP_PROP_FPS)
			  << std::endl;
			  
    if (!cap.isOpened()) {
        std::cerr << "Error: Camera unavailable." << std::endl;
        return -1;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

	cv::Mat frame;
	cap >> frame;

	if (frame.empty()) {
		std::cerr << "ERROR: First camera frame is empty." << std::endl;
		return -1;
	}

	std::cout << "First frame received: "
			  << frame.cols << "x"
			  << frame.rows << std::endl;

    // Drop score threshold down to 0.60 to keep YuNet alive as long as possible
	cv::Ptr<cv::FaceDetectorYN> detector =
		cv::FaceDetectorYN::create(
        yunet_model.string(),
        "",
        frame.size(),
        0.60f,
        0.3f,
        5000
    );;

    cv::Ptr<cv::face::Facemark> facemark = cv::face::FacemarkLBF::create();
    facemark->loadModel(facemark_model.string());

    std::cout << "Memory-locked engine initialized. Ready." << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat faces;
        detector->detect(frame, faces);

        bool face_found_this_frame = false;
        cv::Rect active_face_box;

        if (faces.rows > 0) {
            int x = static_cast<int>(faces.at<float>(0, 0));
            int y = static_cast<int>(faces.at<float>(0, 1));
            int w = static_cast<int>(faces.at<float>(0, 2));
            int h = static_cast<int>(faces.at<float>(0, 3));

            active_face_box = cv::Rect(x, y, w, h) & cv::Rect(0, 0, frame.cols, frame.rows);
            last_valid_face_box = active_face_box; // Store to memory cache
            face_found_this_frame = true;
            face_lost_frame_counter = 0;
        }
        else {
            face_lost_frame_counter++;
            // If we recently had a face box, keep using the cached box position
            if (face_lost_frame_counter <= MAX_FACE_LOST_GRACE_PERIOD && last_valid_face_box.width > 0) {
                active_face_box = last_valid_face_box;
                face_found_this_frame = true;
            }
        }

        std::vector<std::vector<cv::Point2f>> landmarks;
        std::vector<cv::Rect> fit_boxes;
        if (face_found_this_frame) {
            fit_boxes.push_back(active_face_box);
        }

        bool current_frame_eyes_closed = false;
        double current_ear = 0.34;
        bool landmark_success = false;

        // Try extracting landmarks using either current or cached memory bounding box
        if (!fit_boxes.empty() && facemark->fit(frame, fit_boxes, landmarks)) {
            if (!landmarks.empty() && landmarks.at(0).size() >= 48) {
                landmark_success = true;
                const auto& face_pts = landmarks.at(0);

                std::vector<cv::Point2f> right_eye(face_pts.begin() + 36, face_pts.begin() + 42);
                std::vector<cv::Point2f> left_eye(face_pts.begin() + 42, face_pts.begin() + 48);

                double r_ear = calculateEAR(right_eye);
                double l_ear = calculateEAR(left_eye);
                current_ear = (r_ear + l_ear) / 2.0;

                // Draw alignment markers
                for (const auto& pt : right_eye) cv::circle(frame, pt, 2, cv::Scalar(0, 255, 255), -1);
                for (const auto& pt : left_eye) cv::circle(frame, pt, 2, cv::Scalar(0, 255, 255), -1);

                if (r_ear < EAR_CLOSE_THRESHOLD || l_ear < EAR_CLOSE_THRESHOLD) {
                    current_frame_eyes_closed = true;
                }
            }
        }

        // CRITICAL CONTEXT FIX: If face box tracking drops out completely right after an eye narrowing signal, 
        // it means the user closed their eyes tight. Force a closed state.
        if (!landmark_success && face_lost_frame_counter > 0 && face_lost_frame_counter <= MAX_FACE_LOST_GRACE_PERIOD) {
            current_frame_eyes_closed = true;
        }

        // UNBREAKABLE ABSOLUTE STATE MACHINE
        if (current_frame_eyes_closed) {
            if (!is_eye_closed) {
                blink_count++; // Increments EXACTLY once when entering the down-state
                is_eye_closed = true;
            }
        }
        else {
            if (landmark_success && current_ear >= EAR_CLOSE_THRESHOLD) {
                is_eye_closed = false; // Only reset to open if tracking clearly proves they are open
            }
        }

        // RENDER STABLE TEXT OVERLAY
        std::string text = "Blinks: " + std::to_string(blink_count);
        if (landmark_success) {
            text += " | EAR: " + cv::format("%.2f", current_ear);
        }
        else {
            text += " | EAR: RECOVERING";
        }

        if (is_eye_closed) {
            text += " [CLOSED]";
            cv::putText(frame, text, cv::Point(30, 50), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2); // Red Locked Closed text
        }
        else {
            cv::putText(frame, text, cv::Point(30, 50), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2); // Green Open text
        }

        cv::imshow("YuNet + Geometric EAR Blink Tracker", frame);

        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27 || key == 'q' || key == 'Q') break;
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
