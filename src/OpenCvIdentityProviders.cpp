#include "OpenCvIdentityProviders.hpp"

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace dms
{
namespace
{
cv::Mat view(const FaceImageView &image)
{
    if (!image.bgr || image.width <= 0 || image.height <= 0 || image.strideBytes < image.width * 3)
        return {};
    return {image.height,image.width,CV_8UC3,const_cast<std::uint8_t*>(image.bgr),
            static_cast<std::size_t>(image.strideBytes)};
}

OwnedFaceImage owned(const cv::Mat &input)
{
    cv::Mat bgr;if(input.type()==CV_8UC3)bgr=input;else input.convertTo(bgr,CV_8UC3);
    cv::Mat compact=bgr.isContinuous()?bgr:bgr.clone();OwnedFaceImage result;
    result.width=compact.cols;result.height=compact.rows;result.strideBytes=compact.cols*3;
    result.bgr.assign(compact.data,compact.data+compact.total()*compact.elemSize());return result;
}
}

class OpenCvSFaceProvider::Impl
{
  public:
    std::string id,error;cv::Ptr<cv::FaceDetectorYN> detector;cv::Ptr<cv::FaceRecognizerSF> recognizer;
};

OpenCvSFaceProvider::OpenCvSFaceProvider(const std::string &detectorPath,const std::string &recognizerPath,
                                         std::string modelId) noexcept:impl_(std::make_unique<Impl>())
{
    impl_->id=std::move(modelId);try{if(impl_->id.empty())throw std::runtime_error("SFace model ID is empty");
    impl_->detector=cv::FaceDetectorYN::create(detectorPath,"",{320,320},0.9F,0.3F,5000,
      cv::dnn::DNN_BACKEND_OPENCV,cv::dnn::DNN_TARGET_CPU);
    impl_->recognizer=cv::FaceRecognizerSF::create(recognizerPath,"",cv::dnn::DNN_BACKEND_OPENCV,cv::dnn::DNN_TARGET_CPU);
    if(!impl_->detector||!impl_->recognizer)throw std::runtime_error("OpenCV did not create SFace providers");}
    catch(const std::exception &e){impl_->error=e.what();impl_->detector.release();impl_->recognizer.release();}
}
OpenCvSFaceProvider::~OpenCvSFaceProvider()=default;
bool OpenCvSFaceProvider::valid()const noexcept{return impl_&&impl_->error.empty()&&impl_->detector&&impl_->recognizer;}
const std::string &OpenCvSFaceProvider::diagnostic()const noexcept{return impl_->error;}
const std::string &OpenCvSFaceProvider::modelId()const noexcept{return impl_->id;}
FaceAlignmentResult OpenCvSFaceProvider::align(const FaceImageView &sourceFace)
{
    if(!valid())return {false,{},impl_->error};auto image=view(sourceFace);if(image.empty())return {false,{},"invalid source image"};
    try{impl_->detector->setInputSize(image.size());cv::Mat faces;impl_->detector->detect(image,faces);if(faces.empty())return {false,{},"no face detected"};
    int best=0;float area=0;for(int row=0;row<faces.rows;++row){const float candidate=faces.at<float>(row,2)*faces.at<float>(row,3);if(candidate>area){area=candidate;best=row;}}
    cv::Mat aligned;impl_->recognizer->alignCrop(image,faces.row(best),aligned);if(aligned.empty())return {false,{},"face alignment returned empty output"};
    return {true,owned(aligned),"OpenCV SFace five-point alignment"};}catch(const std::exception&e){return {false,{},e.what()};}
}
EmbeddingResult OpenCvSFaceProvider::extract(const FaceImageView &alignedFace)
{
    if(!valid())return {false,0.0F,{},impl_->error};auto image=view(alignedFace);if(image.empty())return {false,0.0F,{},"invalid aligned image"};
    try{cv::Mat feature;impl_->recognizer->feature(image,feature);if(feature.empty())return {false,0.0F,{},"SFace returned an empty embedding"};
    cv::Mat flat=feature.reshape(1,1);if(flat.type()!=CV_32F)flat.convertTo(flat,CV_32F);FaceEmbedding embedding;embedding.modelId=impl_->id;
    embedding.values.assign(flat.ptr<float>(),flat.ptr<float>()+flat.total());return {true,0.0F,std::move(embedding),"OpenCV SFace embedding"};}
    catch(const std::exception&e){return {false,0.0F,{},e.what()};}
}

FaceQualityResult OpenCvDiagnosticFaceQualityProvider::assess(const FaceImageView &alignedFace)
{
    auto image=view(alignedFace);if(image.empty())return {false,0.0F,"invalid aligned image"};
    cv::Mat gray;cv::cvtColor(image,gray,cv::COLOR_BGR2GRAY);cv::Scalar mean,stddev;cv::meanStdDev(gray,mean,stddev);
    cv::Mat laplacian;cv::Laplacian(gray,laplacian,CV_32F);cv::Scalar lapMean,lapStd;cv::meanStdDev(laplacian,lapMean,lapStd);
    const double exposure=std::max(0.0,1.0-std::abs(mean[0]-127.5)/127.5);
    const double sharpness=1.0-std::exp(-(lapStd[0]*lapStd[0])/100.0);const float score=static_cast<float>(std::sqrt(exposure*sharpness));
    std::ostringstream diagnostic;diagnostic<<"diagnostic exposure="<<exposure<<" sharpness="<<sharpness;
    return {true,score,diagnostic.str()};
}

class OpenCvAntiSpoofProvider::Impl
{
  public:
    std::string id,error;OpenCvPadConfig config;cv::dnn::Net net;
};
OpenCvAntiSpoofProvider::OpenCvAntiSpoofProvider(const std::string &modelPath,std::string modelId,
                                                 OpenCvPadConfig config) noexcept:impl_(std::make_unique<Impl>())
{
    impl_->id=std::move(modelId);impl_->config=config;try{if(impl_->id.empty()||!std::isfinite(config.liveProbability)||!std::isfinite(config.spoofProbability)||config.liveProbability<0||config.liveProbability>1||config.spoofProbability<0||config.spoofProbability>1)throw std::runtime_error("invalid PAD provider configuration");
    impl_->net=cv::dnn::readNetFromONNX(modelPath);impl_->net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);impl_->net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);if(impl_->net.empty())throw std::runtime_error("OpenCV returned an empty PAD network");}
    catch(const std::exception&e){impl_->error=e.what();impl_->net=cv::dnn::Net();}
}
OpenCvAntiSpoofProvider::~OpenCvAntiSpoofProvider()=default;
bool OpenCvAntiSpoofProvider::valid()const noexcept{return impl_&&impl_->error.empty()&&!impl_->net.empty();}
const std::string &OpenCvAntiSpoofProvider::diagnostic()const noexcept{return impl_->error;}
const std::string &OpenCvAntiSpoofProvider::modelId()const noexcept{return impl_->id;}
PresentationResult OpenCvAntiSpoofProvider::evaluate(const FaceImageView &face)
{
    if(!valid())return {PresentationState::NotEvaluated,0.0F,impl_->error};auto image=view(face);if(image.empty())return {PresentationState::NotEvaluated,0.0F,"invalid PAD image"};
    try{cv::Mat resized;cv::resize(image,resized,{128,128});cv::cvtColor(resized,resized,cv::COLOR_BGR2RGB);resized.convertTo(resized,CV_32FC3);std::vector<cv::Mat> channels;cv::split(resized,channels);
    const double means[]={151.2405,119.5950,107.8395},scales[]={63.0105,56.4570,55.0035};for(int i=0;i<3;++i)channels[i]=(channels[i]-means[i])/scales[i];cv::merge(channels,resized);
    impl_->net.setInput(cv::dnn::blobFromImage(resized));cv::Mat output=impl_->net.forward().reshape(1,1);if(output.total()!=2)return {PresentationState::Indeterminate,0.0F,"PAD output does not contain two classes"};
    const float a=output.at<float>(0,0),b=output.at<float>(0,1),maximum=std::max(a,b);const double ea=std::exp(a-maximum),eb=std::exp(b-maximum);const float live=static_cast<float>(ea/(ea+eb)),spoof=1.0F-live;
    std::ostringstream diagnostic;diagnostic<<"diagnostic live_probability="<<live<<" spoof_probability="<<spoof;
    if(!impl_->config.thresholdsApproved)return {PresentationState::Indeterminate,live,diagnostic.str()+" thresholds_unapproved"};
    if(live>=impl_->config.liveProbability)return {PresentationState::Live,live,diagnostic.str()};if(spoof>=impl_->config.spoofProbability)return {PresentationState::Spoof,spoof,diagnostic.str()};return {PresentationState::Indeterminate,live,diagnostic.str()};}
    catch(const std::exception&e){return {PresentationState::Indeterminate,0.0F,e.what()};}
}
} // namespace dms
