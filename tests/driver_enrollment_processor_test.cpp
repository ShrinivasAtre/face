#include "DriverEnrollmentProcessor.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace
{
bool check(bool ok,const char *message){if(!ok)std::cerr<<"FAILED: "<<message<<'\n';return ok;}

struct Alignment final : dms::FaceAlignmentProvider
{
    std::vector<std::string> &calls; bool available=true;
    explicit Alignment(std::vector<std::string> &value):calls(value){}
    dms::FaceAlignmentResult align(const dms::FaceImageView &) override
    {calls.push_back("align");return {available,{{0,1,2,3,4,5},2,1,6},available?"":"alignment failed"};}
};
struct Quality final : dms::FaceQualityProvider
{
    std::vector<std::string> &calls; bool available=true;float score=0.9F;
    explicit Quality(std::vector<std::string> &value):calls(value){}
    dms::FaceQualityResult assess(const dms::FaceImageView &) override
    {calls.push_back("quality");return {available,score,available?"":"quality failed"};}
};
struct Pad final : dms::PresentationAttackProvider
{
    std::vector<std::string> &calls;dms::PresentationState state=dms::PresentationState::Live;
    explicit Pad(std::vector<std::string> &value):calls(value){}
    const std::string &modelId() const noexcept override {static const std::string id="pad-test-v1";return id;}
    dms::PresentationResult evaluate(const dms::FaceImageView &) override
    {calls.push_back("pad");return {state,0.9F,"pad result"};}
};
struct Embedding final : dms::FaceEmbeddingProvider
{
    std::vector<std::string> &calls;bool available=true;
    explicit Embedding(std::vector<std::string> &value):calls(value){}
    const std::string &modelId() const noexcept override {static const std::string id="embedding-test-v1";return id;}
    dms::EmbeddingResult extract(const dms::FaceImageView &) override
    {calls.push_back("embedding");return {available,0.9F,{{0.1F,0.2F},modelId()},available?"":"embedding failed"};}
};
}

int main()
{
    using namespace dms;std::vector<std::string> calls;Alignment alignment(calls);Quality quality(calls);Pad pad(calls);Embedding embedding(calls);
    const std::uint8_t pixels[6]{};FaceImageView input{pixels,2,1,6};
    DriverEnrollmentProcessor diagnostic{{0.5F,false},alignment,quality,pad,embedding};
    auto result=diagnostic.evaluate(input);
    if(!check(result.decision==EnrollmentDecision::ThresholdApprovalRequired,"unapproved gate cannot accept")||
       !check(result.embedding.modelId==embedding.modelId(),"diagnostic embedding returned")||
       !check(calls==std::vector<std::string>({"align","quality","pad","embedding"}),"provider order"))return 1;

    calls.clear();pad.state=PresentationState::Spoof;result=diagnostic.evaluate(input);
    if(!check(result.decision==EnrollmentDecision::SpoofRejected,"spoof rejected")||
       !check(calls==std::vector<std::string>({"align","quality","pad"}),"spoof blocks embedding"))return 1;

    calls.clear();pad.state=PresentationState::Indeterminate;result=diagnostic.evaluate(input);
    if(!check(result.decision==EnrollmentDecision::PresentationUnavailable,"indeterminate PAD rejected")||
       !check(calls==std::vector<std::string>({"align","quality","pad"}),"indeterminate blocks embedding"))return 1;

    calls.clear();pad.state=PresentationState::Live;quality.score=0.4F;result=diagnostic.evaluate(input);
    if(!check(result.decision==EnrollmentDecision::QualityRejected,"low quality rejected")||
       !check(calls==std::vector<std::string>({"align","quality"}),"quality blocks PAD and embedding"))return 1;

    calls.clear();quality.score=0.9F;DriverEnrollmentProcessor approved{{0.5F,true},alignment,quality,pad,embedding};result=approved.evaluate(input);
    if(!check(result.accepted(),"approved gates accept")||!check(result.embedding.values.size()==2,"embedding retained"))return 1;

    calls.clear();result=approved.evaluate({});
    if(!check(result.decision==EnrollmentDecision::InvalidInput,"invalid input rejected")||!check(calls.empty(),"invalid input calls no providers"))return 1;
    std::cout<<"driver enrollment processor test PASSED\n";return 0;
}
