#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
struct Options
{
    std::string detector;
    std::string model;
    std::string image;
    int warmup = 3;
    int iterations = 20;
};

Options parse(int argc, char **argv)
{
    Options options;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        const auto split = argument.find('=');
        if (split == std::string::npos)
            throw std::runtime_error("Expected --name=value: " + argument);
        const auto name = argument.substr(0, split);
        const auto value = argument.substr(split + 1);
        if (name == "--detector") options.detector = value;
        else if (name == "--model") options.model = value;
        else if (name == "--image") options.image = value;
        else if (name == "--warmup") options.warmup = std::stoi(value);
        else if (name == "--iterations") options.iterations = std::stoi(value);
        else throw std::runtime_error("Unknown option: " + name);
    }
    if (options.detector.empty() || options.model.empty() || options.image.empty())
        throw std::runtime_error("detector, model, and image are required");
    if (options.warmup < 0 || options.iterations <= 0 || options.iterations > 10000)
        throw std::runtime_error("warmup must be non-negative and iterations within [1,10000]");
    return options;
}

cv::Mat faceCrop(const cv::Mat &image, const cv::Ptr<cv::FaceDetectorYN> &detector)
{
    detector->setInputSize(image.size());
    cv::Mat faces;
    detector->detect(image, faces);
    if (faces.empty())
        throw std::runtime_error("No face detected in input image");
    const auto &face = faces.row(0);
    const int x = std::max(0, static_cast<int>(face.at<float>(0, 0)));
    const int y = std::max(0, static_cast<int>(face.at<float>(0, 1)));
    const int width = std::min(image.cols - x, static_cast<int>(face.at<float>(0, 2)));
    const int height = std::min(image.rows - y, static_cast<int>(face.at<float>(0, 3)));
    if (width <= 0 || height <= 0)
        throw std::runtime_error("Face detector returned an invalid box");
    return image(cv::Rect{x, y, width, height}).clone();
}

cv::Mat padBlob(const cv::Mat &crop)
{
    cv::Mat resized;
    cv::resize(crop, resized, {128, 128});
    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    resized.convertTo(resized, CV_32FC3);
    std::vector<cv::Mat> channels;
    cv::split(resized, channels);
    const double means[] = {151.2405, 119.5950, 107.8395};
    const double scales[] = {63.0105, 56.4570, 55.0035};
    for (int channel = 0; channel < 3; ++channel)
        channels[channel] = (channels[channel] - means[channel]) / scales[channel];
    cv::merge(channels, resized);
    return cv::dnn::blobFromImage(resized);
}

cv::Mat infer(cv::dnn::Net &net, const cv::Mat &blob)
{
    net.setInput(blob);
    const auto output = net.forward();
    if (output.total() != 2)
        throw std::runtime_error("PAD model did not return two class scores");
    return output.reshape(1, 1).clone();
}

double percentile(std::vector<double> values, double fraction)
{
    std::sort(values.begin(), values.end());
    return values[static_cast<std::size_t>(fraction * static_cast<double>(values.size() - 1))];
}
} // namespace

int main(int argc, char **argv)
{
    try
    {
        const auto options = parse(argc, argv);
        const auto image = cv::imread(options.image);
        if (image.empty())
            throw std::runtime_error("Unable to decode input image");
        auto detector = cv::FaceDetectorYN::create(options.detector, "", {320, 320}, 0.9F, 0.3F, 5000,
                                                    cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
        auto net = cv::dnn::readNetFromONNX(options.model);
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        const auto blob = padBlob(faceCrop(image, detector));
        for (int index = 0; index < options.warmup; ++index)
            (void)infer(net, blob);
        std::vector<double> latencyMs;
        cv::Mat output;
        for (int index = 0; index < options.iterations; ++index)
        {
            const auto start = std::chrono::steady_clock::now();
            output = infer(net, blob);
            const auto stop = std::chrono::steady_clock::now();
            latencyMs.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
        }
        const double mean = std::accumulate(latencyMs.begin(), latencyMs.end(), 0.0) / latencyMs.size();
        std::cout << std::fixed << std::setprecision(6)
                  << "{\"schema\":1,\"runtime\":\"opencv-" << CV_VERSION
                  << "\",\"backend\":\"opencv-cpu\",\"warmup\":" << options.warmup
                  << ",\"iterations\":" << options.iterations << ",\"class0_real_score\":"
                  << output.at<float>(0, 0) << ",\"class1_spoof_score\":" << output.at<float>(0, 1)
                  << ",\"mean_inference_ms\":" << mean
                  << ",\"p50_inference_ms\":" << percentile(latencyMs, 0.50)
                  << ",\"p95_inference_ms\":" << percentile(latencyMs, 0.95)
                  << ",\"classification_evaluable\":false}\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "PAD baseline failed: " << error.what() << '\n';
        return 1;
    }
}
