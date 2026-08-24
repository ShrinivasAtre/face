#include "PfldLandmarkProvider.hpp"

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>

namespace
{
constexpr int kInputSize = 112;
constexpr float kPaddingRatio = 0.10f;

bool finite(float value) { return std::isfinite(value); }
}  // namespace

class PfldLandmarkProvider::Impl
{
public:
    cv::dnn::Net net;
};

PfldLandmarkProvider::PfldLandmarkProvider()
    : impl_(std::make_unique<Impl>()) {}

PfldLandmarkProvider::~PfldLandmarkProvider() = default;

bool PfldLandmarkProvider::initialize(const std::string& modelPath)
{
    impl_->net = {};
    try
    {
        impl_->net = cv::dnn::readNetFromONNX(modelPath);
        if (impl_->net.empty())
        {
            std::cerr << "ERROR: PFLD ONNX network is empty\n";
            return false;
        }
        return true;
    }
    catch (const cv::Exception& error)
    {
        std::cerr << "ERROR: PFLD initialization failed: "
                  << error.what() << '\n';
        impl_->net = {};
        return false;
    }
}

bool PfldLandmarkProvider::detect(
    const cv::Mat& frame,
    const cv::Rect& faceBox,
    std::vector<FaceLandmark>& result)
{
    result.clear();
    if (impl_->net.empty() || frame.empty())
        return false;

    const cv::Rect bounds(0, 0, frame.cols, frame.rows);
    const cv::Rect clipped = faceBox & bounds;
    if (clipped.width <= 0 || clipped.height <= 0)
        return false;

    const int padding = static_cast<int>(std::lround(
        std::max(clipped.width, clipped.height) * kPaddingRatio));
    const cv::Rect expanded(
        clipped.x - padding,
        clipped.y - padding,
        clipped.width + 2 * padding,
        clipped.height + 2 * padding);
    const cv::Rect cropBox = expanded & bounds;
    if (cropBox.width <= 0 || cropBox.height <= 0)
        return false;

    const float scale = std::min(
        static_cast<float>(kInputSize) / cropBox.width,
        static_cast<float>(kInputSize) / cropBox.height);
    const int scaledWidth = std::max(
        1, static_cast<int>(std::lround(cropBox.width * scale)));
    const int scaledHeight = std::max(
        1, static_cast<int>(std::lround(cropBox.height * scale)));
    const int padX = (kInputSize - scaledWidth) / 2;
    const int padY = (kInputSize - scaledHeight) / 2;

    cv::Mat resized;
    cv::resize(frame(cropBox), resized, cv::Size(scaledWidth, scaledHeight));
    cv::Mat input(kInputSize, kInputSize, frame.type(), cv::Scalar::all(128));
    resized.copyTo(input(cv::Rect(padX, padY, scaledWidth, scaledHeight)));

    try
    {
        cv::Mat blob = cv::dnn::blobFromImage(
            input, 1.0 / 255.0, cv::Size(kInputSize, kInputSize),
            cv::Scalar(), true, false, CV_32F);
        impl_->net.setInput(blob);
        cv::Mat output = impl_->net.forward();
        if (output.empty() || output.total() != landmarkCount() * 2)
            return false;

        cv::Mat values = output.reshape(1, 1);
        if (values.type() != CV_32F)
            values.convertTo(values, CV_32F);

        result.reserve(landmarkCount());
        for (std::size_t index = 0; index < landmarkCount(); ++index)
        {
            const int offset = static_cast<int>(index * 2);
            const float normalizedX = values.at<float>(0, offset);
            const float normalizedY = values.at<float>(0, offset + 1);
            if (!finite(normalizedX) || !finite(normalizedY))
            {
                result.clear();
                return false;
            }
            const float x = cropBox.x +
                (normalizedX * kInputSize - padX) / scale;
            const float y = cropBox.y +
                (normalizedY * kInputSize - padY) / scale;
            result.push_back({x, y, 0.0f});
        }
        return true;
    }
    catch (const cv::Exception& error)
    {
        std::cerr << "ERROR: PFLD inference failed: " << error.what() << '\n';
        result.clear();
        return false;
    }
}
