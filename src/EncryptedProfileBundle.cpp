#include "EncryptedProfileBundle.hpp"

#include <cstring>
#include <limits>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#endif

namespace dms
{
namespace
{
constexpr std::uint32_t minimumIterations = 600000;
constexpr std::size_t saltSize = 16, nonceSize = 12, tagSize = 16, keySize = 32;
constexpr std::size_t maximumPlaintext = 200 * 1024 * 1024;
constexpr char magic[] = "DMSBND01";
void put32(std::vector<std::uint8_t> &out, std::uint32_t v){for(int s=0;s<32;s+=8)out.push_back(static_cast<std::uint8_t>(v>>s));}
void put64(std::vector<std::uint8_t> &out, std::uint64_t v){for(int s=0;s<64;s+=8)out.push_back(static_cast<std::uint8_t>(v>>s));}
bool get32(const std::vector<std::uint8_t>&b,std::size_t&o,std::uint32_t&v){if(b.size()-o<4)return false;v=0;for(int i=0;i<4;++i)v|=static_cast<std::uint32_t>(b[o++])<<(8*i);return true;}
bool get64(const std::vector<std::uint8_t>&b,std::size_t&o,std::uint64_t&v){if(b.size()-o<8)return false;v=0;for(int i=0;i<8;++i)v|=static_cast<std::uint64_t>(b[o++])<<(8*i);return true;}

#ifdef _WIN32
struct Algorithm { BCRYPT_ALG_HANDLE value=nullptr; ~Algorithm(){if(value)BCryptCloseAlgorithmProvider(value,0);} };
struct Key { BCRYPT_KEY_HANDLE value=nullptr; std::vector<std::uint8_t> object; ~Key(){if(value)BCryptDestroyKey(value);} };

bool derive(const std::string &pass,const std::uint8_t *salt,std::uint32_t iterations,std::uint8_t *key,std::string &error)
{
    Algorithm sha;
    if(BCryptOpenAlgorithmProvider(&sha.value,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0){error="unable to open SHA-256 provider";return false;}
    if(BCryptDeriveKeyPBKDF2(sha.value,reinterpret_cast<PUCHAR>(const_cast<char*>(pass.data())),static_cast<ULONG>(pass.size()),const_cast<PUCHAR>(salt),static_cast<ULONG>(saltSize),iterations,key,static_cast<ULONG>(keySize),0)<0){error="PBKDF2 failed";return false;}
    return true;
}

bool makeAes(const std::uint8_t *raw,Algorithm &aes,Key &key,std::string &error)
{
    if(BCryptOpenAlgorithmProvider(&aes.value,BCRYPT_AES_ALGORITHM,nullptr,0)<0||BCryptSetProperty(aes.value,BCRYPT_CHAINING_MODE,reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),sizeof(BCRYPT_CHAIN_MODE_GCM),0)<0){error="unable to configure AES-GCM";return false;}
    ULONG bytes=0,size=0;if(BCryptGetProperty(aes.value,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&bytes,0)<0){error="unable to query AES key storage";return false;}
    key.object.resize(size);if(BCryptGenerateSymmetricKey(aes.value,&key.value,key.object.data(),size,const_cast<PUCHAR>(raw),static_cast<ULONG>(keySize),0)<0){error="unable to create AES key";return false;}return true;
}
#endif
} // namespace

bool BundleCryptoConfig::validate(std::string &error) const noexcept
{
    if(pbkdf2Iterations<minimumIterations){error="PBKDF2 iterations must be at least 600000";return false;}error.clear();return true;
}

bool encryptProfileBundle(const std::vector<std::uint8_t>&plain,const std::string&pass,const BundleCryptoConfig&config,std::vector<std::uint8_t>&bundle,std::string&error) noexcept
{
    bundle.clear();error.clear();
#ifndef _WIN32
    (void)plain;(void)pass;(void)config;error="AES-GCM bundle provider is not configured on this platform";return false;
#else
    try {
        if(!config.validate(error)||pass.size()<12||pass.size()>1024||plain.empty()||plain.size()>maximumPlaintext){if(error.empty())error="invalid passphrase or plaintext size";return false;}
        std::uint8_t salt[saltSize],nonce[nonceSize],raw[keySize]{};
        if(BCryptGenRandom(nullptr,salt,sizeof salt,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0||BCryptGenRandom(nullptr,nonce,sizeof nonce,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0){error="secure random generation failed";return false;}
        if(!derive(pass,salt,config.pbkdf2Iterations,raw,error)){SecureZeroMemory(raw,sizeof raw);return false;}
        bundle.assign(std::begin(magic),std::end(magic)-1);put32(bundle,1);put32(bundle,config.pbkdf2Iterations);bundle.insert(bundle.end(),salt,salt+saltSize);bundle.insert(bundle.end(),nonce,nonce+nonceSize);put64(bundle,plain.size());
        const auto aadSize=bundle.size();std::vector<std::uint8_t> cipher(plain.size()),tag(tagSize);Algorithm aes;Key key;
        if(!makeAes(raw,aes,key,error)){SecureZeroMemory(raw,sizeof raw);return false;}SecureZeroMemory(raw,sizeof raw);
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;BCRYPT_INIT_AUTH_MODE_INFO(info);info.pbNonce=nonce;info.cbNonce=nonceSize;info.pbAuthData=bundle.data();info.cbAuthData=static_cast<ULONG>(aadSize);info.pbTag=tag.data();info.cbTag=tagSize;ULONG written=0;
        if(BCryptEncrypt(key.value,const_cast<PUCHAR>(plain.data()),static_cast<ULONG>(plain.size()),&info,nullptr,0,cipher.data(),static_cast<ULONG>(cipher.size()),&written,0)<0||written!=cipher.size()){bundle.clear();error="AES-GCM encryption failed";return false;}
        bundle.insert(bundle.end(),cipher.begin(),cipher.end());bundle.insert(bundle.end(),tag.begin(),tag.end());return true;
    } catch(...) {bundle.clear();error="profile bundle encryption failed";return false;}
#endif
}

bool decryptProfileBundle(const std::vector<std::uint8_t>&bundle,const std::string&pass,std::vector<std::uint8_t>&plain,std::string&error) noexcept
{
    plain.clear();error.clear();
#ifndef _WIN32
    (void)bundle;(void)pass;error="AES-GCM bundle provider is not configured on this platform";return false;
#else
    try {
        const std::size_t fixed=8+4+4+saltSize+nonceSize+8;if(bundle.size()<fixed+tagSize||std::memcmp(bundle.data(),magic,8)!=0||pass.size()<12||pass.size()>1024){error="invalid encrypted bundle";return false;}
        std::size_t offset=8;std::uint32_t version=0,iterations=0;std::uint64_t cipherSize=0;if(!get32(bundle,offset,version)||version!=1||!get32(bundle,offset,iterations)||iterations<minimumIterations){error="unsupported encrypted bundle";return false;}
        const auto *salt=bundle.data()+offset;offset+=saltSize;const auto *nonce=bundle.data()+offset;offset+=nonceSize;if(!get64(bundle,offset,cipherSize)||cipherSize==0||cipherSize>maximumPlaintext||cipherSize!=bundle.size()-fixed-tagSize){error="invalid encrypted bundle length";return false;}
        const auto aadSize=offset;const auto *cipher=bundle.data()+offset;const auto *tag=cipher+cipherSize;std::uint8_t raw[keySize]{};if(!derive(pass,salt,iterations,raw,error)){SecureZeroMemory(raw,sizeof raw);return false;}
        Algorithm aes;Key key;if(!makeAes(raw,aes,key,error)){SecureZeroMemory(raw,sizeof raw);return false;}SecureZeroMemory(raw,sizeof raw);plain.resize(static_cast<std::size_t>(cipherSize));
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;BCRYPT_INIT_AUTH_MODE_INFO(info);info.pbNonce=const_cast<PUCHAR>(nonce);info.cbNonce=nonceSize;info.pbAuthData=const_cast<PUCHAR>(bundle.data());info.cbAuthData=static_cast<ULONG>(aadSize);info.pbTag=const_cast<PUCHAR>(tag);info.cbTag=tagSize;ULONG written=0;
        if(BCryptDecrypt(key.value,const_cast<PUCHAR>(cipher),static_cast<ULONG>(cipherSize),&info,nullptr,0,plain.data(),static_cast<ULONG>(plain.size()),&written,0)<0||written!=plain.size()){plain.clear();error="authentication failed or passphrase is incorrect";return false;}return true;
    } catch(...) {plain.clear();error="profile bundle decryption failed";return false;}
#endif
}
} // namespace dms
