#include "ProcessingRoiAdapter.hpp"

#include <iostream>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    dms::ProcessingRoi roi;
    roi.enabled = true;
    roi.x = 0.25;
    roi.y = 0.10;
    roi.width = 0.50;
    roi.height = 0.80;
    roi.minimumFaceCoverage = 0.80;
    cv::Mat full(480, 640, CV_8UC3, cv::Scalar(1, 2, 3));
    cv::Mat selected;
    ProcessingRoiContext context;
    std::string error;
    if (!check(selectProcessingFrame(full, roi, selected, context, error), "select ROI") ||
        !check(selected.cols == 320 && selected.rows == 384, "selected dimensions") ||
        !check(context.processingRectangle == cv::Rect(160, 48, 320, 384), "pixel rectangle")) return 1;

    FaceResult accepted;
    accepted.detected = true;
    accepted.landmarksValid = true;
    accepted.faceBox = {20, 30, 100, 120};
    accepted.landmarks = {{25.0F, 35.0F, 1.0F}, {80.0F, 90.0F, 2.0F}};
    if (!check(restoreProcessingResult(context, roi, accepted), "accept covered face") ||
        !check(accepted.faceBox == cv::Rect(180, 78, 100, 120), "restore face box") ||
        !check(accepted.landmarks[0].x == 185.0F && accepted.landmarks[0].y == 83.0F,
               "restore landmark coordinates") ||
        !check(accepted.landmarks[0].z == 1.0F, "preserve landmark depth")) return 1;

    FaceResult marginal;
    marginal.detected = true;
    marginal.landmarksValid = true;
    marginal.faceBox = {-30, 20, 100, 100};
    marginal.landmarks = {{10.0F, 30.0F, 0.0F}};
    if (!check(!restoreProcessingResult(context, roi, marginal), "reject low coverage") ||
        !check(!marginal.detected && marginal.landmarks.empty(), "clear rejected result")) return 1;

    FaceResult noDetection;
    if (!check(restoreProcessingResult(context, roi, noDetection), "no detection remains valid")) return 1;

    dms::ProcessingRoi disabled;
    if (!check(selectProcessingFrame(full, disabled, selected, context, error), "disabled ROI") ||
        !check(selected.size() == full.size(), "disabled ROI selects full frame") ||
        !check(selected.data == full.data && selected.step == full.step,
               "disabled ROI preserves full-frame bytes and layout")) return 1;
    FaceResult legacyEdge;
    legacyEdge.detected = true;
    legacyEdge.faceBox = {-30, 20, 100, 100};
    legacyEdge.landmarks = {{1.0F, 2.0F, 3.0F}};
    if (!check(restoreProcessingResult(context, disabled, legacyEdge) && legacyEdge.detected,
               "disabled ROI preserves legacy detections") ||
        !check(legacyEdge.faceBox == cv::Rect(-30, 20, 100, 100) &&
               legacyEdge.landmarks[0].x == 1.0F && legacyEdge.landmarks[0].y == 2.0F,
               "disabled ROI preserves result coordinates")) return 1;

    cv::Mat empty;
    if (!check(!selectProcessingFrame(empty, disabled, selected, context, error) && !error.empty(),
               "empty frame rejected")) return 1;

    std::cout << "Processing ROI adapter tests PASSED\n";
    return 0;
}
