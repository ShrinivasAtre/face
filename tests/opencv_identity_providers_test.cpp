#include "OpenCvIdentityProviders.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace { bool check(bool ok,const char*m){if(!ok)std::cerr<<"FAILED: "<<m<<'\n';return ok;} }

int main()
{
    using namespace dms;
    std::vector<std::uint8_t> pixels(112*112*3);
    for(std::size_t i=0;i<pixels.size();++i)pixels[i]=static_cast<std::uint8_t>((i*37)%256);
    const FaceImageView image{pixels.data(),112,112,112*3};
    OpenCvDiagnosticFaceQualityProvider quality;const auto assessed=quality.assess(image);
    if(!check(assessed.available,"quality available")||
       !check(std::isfinite(assessed.score)&&assessed.score>=0.0F&&assessed.score<=1.0F,"quality bounded")||
       !check(!quality.assess({}).available,"invalid quality input rejected"))return 1;

    OpenCvSFaceProvider sface("missing-detector.onnx","missing-recognizer.onnx","test-sface");
    if(!check(!sface.valid(),"missing SFace assets rejected")||
       !check(!sface.align(image).available,"invalid SFace provider fails closed"))return 1;
    OpenCvAntiSpoofProvider pad("missing-pad.onnx","test-pad");
    if(!check(!pad.valid(),"missing PAD asset rejected")||
       !check(pad.evaluate(image).state==PresentationState::NotEvaluated,"invalid PAD provider fails closed"))return 1;
    std::cout<<"OpenCV identity provider tests PASSED\n";return 0;
}
