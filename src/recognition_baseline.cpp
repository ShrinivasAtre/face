#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>
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
    std::string recognizer;
    std::string enrollment;
    std::string genuine;
    std::string impostor;
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
        else if (name == "--recognizer") options.recognizer = value;
        else if (name == "--enrollment") options.enrollment = value;
        else if (name == "--genuine") options.genuine = value;
        else if (name == "--impostor") options.impostor = value;
        else if (name == "--warmup") options.warmup = std::stoi(value);
        else if (name == "--iterations") options.iterations = std::stoi(value);
        else throw std::runtime_error("Unknown option: " + name);
    }
    if (options.detector.empty() || options.recognizer.empty() || options.enrollment.empty() ||
        options.genuine.empty() || options.impostor.empty())
        throw std::runtime_error("detector, recognizer, enrollment, genuine, and impostor are required");
    if (options.warmup < 0 || options.iterations <= 0 || options.iterations > 10000)
        throw std::runtime_error("warmup must be non-negative and iterations within [1,10000]");
    return options;
}

cv::Mat firstFace(const cv::Mat &image, const cv::Ptr<cv::FaceDetectorYN> &detector)
{
    detector->setInputSize(image.size());
    cv::Mat faces;
    detector->detect(image, faces);
    if (faces.empty())
        throw std::runtime_error("No face detected in an input image");
    return faces.row(0).clone();
}

cv::Mat feature(const cv::Mat &image, const cv::Mat &face, const cv::Ptr<cv::FaceRecognizerSF> &recognizer)
{
    cv::Mat aligned;
    cv::Mat result;
    recognizer->alignCrop(image, face, aligned);
    recognizer->feature(aligned, result);
    if (result.empty())
        throw std::runtime_error("Recognizer returned an empty embedding");
    return result.clone();
}

double percentile(std::vector<double> values, double fraction)
{
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>(fraction * static_cast<double>(values.size() - 1));
    return values[index];
}
} // namespace

int main(int argc, char **argv)
{
    try
    {
        const auto options = parse(argc, argv);
        const auto enrollment = cv::imread(options.enrollment);
        const auto genuine = cv::imread(options.genuine);
        const auto impostor = cv::imread(options.impostor);
        if (enrollment.empty() || genuine.empty() || impostor.empty())
            throw std::runtime_error("Unable to decode one or more input images");

        auto detector = cv::FaceDetectorYN::create(options.detector, "", {320, 320}, 0.9F, 0.3F, 5000,
                                                    cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
        auto recognizer = cv::FaceRecognizerSF::create(options.recognizer, "", cv::dnn::DNN_BACKEND_OPENCV,
                                                       cv::dnn::DNN_TARGET_CPU);
        const auto enrollmentFace = firstFace(enrollment, detector);
        const auto genuineFace = firstFace(genuine, detector);
        const auto impostorFace = firstFace(impostor, detector);
        const auto enrollmentFeature = feature(enrollment, enrollmentFace, recognizer);
        const auto genuineFeature = feature(genuine, genuineFace, recognizer);
        const auto impostorFeature = feature(impostor, impostorFace, recognizer);

        for (int index = 0; index < options.warmup; ++index)
            (void)feature(genuine, genuineFace, recognizer);
        std::vector<double> latencyMs;
        latencyMs.reserve(static_cast<std::size_t>(options.iterations));
        for (int index = 0; index < options.iterations; ++index)
        {
            const auto start = std::chrono::steady_clock::now();
            (void)feature(genuine, genuineFace, recognizer);
            const auto stop = std::chrono::steady_clock::now();
            latencyMs.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
        }
        const double mean = std::accumulate(latencyMs.begin(), latencyMs.end(), 0.0) / latencyMs.size();
        const double genuineScore = recognizer->match(enrollmentFeature, genuineFeature,
                                                      cv::FaceRecognizerSF::FR_COSINE);
        const double impostorScore = recognizer->match(enrollmentFeature, impostorFeature,
                                                       cv::FaceRecognizerSF::FR_COSINE);
        std::cout << std::fixed << std::setprecision(6)
                  << "{\"schema\":1,\"runtime\":\"opencv-" << CV_VERSION
                  << "\",\"backend\":\"opencv-cpu\",\"warmup\":" << options.warmup
                  << ",\"iterations\":" << options.iterations << ",\"embedding_dimensions\":"
                  << enrollmentFeature.total() << ",\"genuine_cosine\":" << genuineScore
                  << ",\"impostor_cosine\":" << impostorScore << ",\"mean_embedding_ms\":" << mean
                  << ",\"p50_embedding_ms\":" << percentile(latencyMs, 0.50)
                  << ",\"p95_embedding_ms\":" << percentile(latencyMs, 0.95) << "}\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "recognition baseline failed: " << error.what() << '\n';
        return 1;
    }
}
