#include "DriverProfileDatabase.hpp"
#include "EncryptedProfileBundle.hpp"

#include <iostream>

namespace { bool check(bool ok,const char*m){if(!ok)std::cerr<<"FAILED: "<<m<<'\n';return ok;} }

int main()
{
    using namespace dms; std::string error; DriverProfileDatabase db;
    if(!check(db.create("driver-01","Driver One",error),"create")||
       !check(!db.create("driver-01","Duplicate",error),"duplicate rejected")||
       !check(db.addImage("driver-01",{EnrollmentSource::Photo,0.9F,{1,2,3,4}},error),"add image")||
       !check(db.addEmbedding("driver-01",{{0.1F,0.2F,0.3F},"mock-v1"},error),"add embedding")) return 1;
    auto bytes=db.serialize(error);auto restored=DriverProfileDatabase::deserialize(bytes,error);
    if(!check(restored.has_value(),"deserialize")||!check(restored->profiles().size()==1,"profile count")||
       !check(restored->find("driver-01")->images.size()==1,"image retained")||
       !check(restored->find("driver-01")->embeddings.size()==1,"embedding retained")) return 1;
    auto imported=*restored;
    if(!check(!db.importProfile(imported.profiles()[0],ImportConflict::Reject,"",error),"conflict rejected")||
       !check(db.importProfile(imported.profiles()[0],ImportConflict::NewAnonymousId,"driver-02",error),"new ID import")||
       !check(db.erase("driver-01",error)&&db.find("driver-01")==nullptr,"delete removes profile")) return 1;

    BundleCryptoConfig weak{100};
    if(!check(!weak.validate(error),"weak KDF rejected")) return 1;
#ifdef _WIN32
    std::vector<std::uint8_t> bundle,plain;
    if(!check(encryptProfileBundle(bytes,"correct horse battery",{},bundle,error),"encrypt")||
       !check(bundle!=bytes,"not plaintext")||
       !check(decryptProfileBundle(bundle,"correct horse battery",plain,error)&&plain==bytes,"round trip")||
       !check(!decryptProfileBundle(bundle,"incorrect passphrase",plain,error),"wrong passphrase rejected")) return 1;
    bundle[bundle.size()/2]^=1;
    if(!check(!decryptProfileBundle(bundle,"correct horse battery",plain,error),"tamper rejected")) return 1;
#else
    std::vector<std::uint8_t> bundle;
    if(!check(!encryptProfileBundle(bytes,"correct horse battery",{},bundle,error),"missing provider fails closed")) return 1;
#endif
    std::cout<<"driver profile database test PASSED\n";return 0;
}
