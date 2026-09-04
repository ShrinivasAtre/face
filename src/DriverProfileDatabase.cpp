#include "DriverProfileDatabase.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>

namespace dms
{
namespace
{
constexpr char magic[] = "DMSPRF01";
constexpr std::size_t maximumString = 1024;
constexpr std::size_t maximumEmbeddingDimensions = 4096;

bool validId(const std::string &value)
{
    if (value.empty() || value.size() > 64) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '-' || c == '_';
    });
}

void put32(std::vector<std::uint8_t> &out, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) out.push_back(static_cast<std::uint8_t>(value >> shift));
}
void put64(std::vector<std::uint8_t> &out, std::uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8) out.push_back(static_cast<std::uint8_t>(value >> shift));
}
void putString(std::vector<std::uint8_t> &out, const std::string &value)
{
    put32(out, static_cast<std::uint32_t>(value.size())); out.insert(out.end(), value.begin(), value.end());
}

class Reader
{
  public:
    explicit Reader(const std::vector<std::uint8_t> &bytes) : bytes_(bytes) {}
    bool take(void *output, std::size_t count)
    {
        if (count > bytes_.size() - offset_) return false;
        std::memcpy(output, bytes_.data() + offset_, count); offset_ += count; return true;
    }
    bool u32(std::uint32_t &value)
    {
        std::uint8_t b[4]; if (!take(b, 4)) return false; value = 0;
        for (int i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(b[i]) << (8 * i); return true;
    }
    bool u64(std::uint64_t &value)
    {
        std::uint8_t b[8]; if (!take(b, 8)) return false; value = 0;
        for (int i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(b[i]) << (8 * i); return true;
    }
    bool string(std::string &value)
    {
        std::uint32_t size; if (!u32(size) || size > maximumString || size > bytes_.size() - offset_) return false;
        value.assign(reinterpret_cast<const char *>(bytes_.data() + offset_), size); offset_ += size; return true;
    }
    bool finished() const { return offset_ == bytes_.size(); }
  private:
    const std::vector<std::uint8_t> &bytes_; std::size_t offset_ = 0;
};

bool validateProfile(const StoredDriverProfile &profile, std::string &error)
{
    if (!validId(profile.driverId)) error = "invalid driver ID";
    else if (profile.displayName.size() > 256) error = "display name is too long";
    else if (profile.images.size() > DriverProfileDatabase::maximumImagesPerProfile) error = "too many images";
    else if (profile.embeddings.size() > DriverProfileDatabase::maximumEmbeddingsPerProfile) error = "too many embeddings";
    else {
        for (const auto &image : profile.images)
            if (image.encodedImage.empty() || image.encodedImage.size() > DriverProfileDatabase::maximumEncodedImageBytes ||
                !std::isfinite(image.quality) || image.quality < 0.0F || image.quality > 1.0F)
                { error = "invalid enrollment image"; break; }
        for (const auto &embedding : profile.embeddings)
            if (embedding.modelId.empty() || embedding.modelId.size() > 256 || embedding.values.empty() ||
                embedding.values.size() > maximumEmbeddingDimensions ||
                !std::all_of(embedding.values.begin(), embedding.values.end(), [](float v){return std::isfinite(v);}))
                { error = "invalid embedding"; break; }
    }
    return error.empty();
}
} // namespace

const StoredDriverProfile *DriverProfileDatabase::find(const std::string &id) const noexcept
{
    const auto it = std::find_if(profiles_.begin(), profiles_.end(), [&](const auto &p){return p.driverId == id;});
    return it == profiles_.end() ? nullptr : &*it;
}

bool DriverProfileDatabase::create(std::string id, std::string name, std::string &error)
{
    error.clear(); StoredDriverProfile profile{std::move(id), std::move(name)};
    if (!validateProfile(profile, error)) return false;
    if (find(profile.driverId)) { error = "driver ID already exists"; return false; }
    if (profiles_.size() >= kMaximumDriverProfiles) { error = "profile limit reached"; return false; }
    profiles_.push_back(std::move(profile)); return true;
}

bool DriverProfileDatabase::erase(const std::string &id, std::string &error)
{
    error.clear(); const auto before = profiles_.size();
    profiles_.erase(std::remove_if(profiles_.begin(), profiles_.end(), [&](const auto &p){return p.driverId == id;}), profiles_.end());
    if (profiles_.size() == before) { error = "driver ID not found"; return false; }
    return true;
}

bool DriverProfileDatabase::addImage(const std::string &id, EnrollmentImage image, std::string &error)
{
    error.clear(); auto it = std::find_if(profiles_.begin(), profiles_.end(), [&](const auto &p){return p.driverId == id;});
    if (it == profiles_.end()) { error = "driver ID not found"; return false; }
    auto copy = *it; copy.images.push_back(std::move(image)); if (!validateProfile(copy, error)) return false;
    copy.revision++; *it = std::move(copy); return true;
}

bool DriverProfileDatabase::addEmbedding(const std::string &id, FaceEmbedding embedding, std::string &error)
{
    error.clear(); auto it = std::find_if(profiles_.begin(), profiles_.end(), [&](const auto &p){return p.driverId == id;});
    if (it == profiles_.end()) { error = "driver ID not found"; return false; }
    auto copy = *it; copy.embeddings.push_back(std::move(embedding)); if (!validateProfile(copy, error)) return false;
    copy.revision++; *it = std::move(copy); return true;
}

bool DriverProfileDatabase::importProfile(StoredDriverProfile profile, ImportConflict conflict, std::string newId, std::string &error)
{
    error.clear(); if (!validateProfile(profile, error)) return false;
    auto it = std::find_if(profiles_.begin(), profiles_.end(), [&](const auto &p){return p.driverId == profile.driverId;});
    if (it == profiles_.end()) { if (profiles_.size() >= kMaximumDriverProfiles) {error="profile limit reached";return false;} profiles_.push_back(std::move(profile)); return true; }
    if (conflict == ImportConflict::Reject) { error = "import conflict"; return false; }
    if (conflict == ImportConflict::NewAnonymousId) {
        if (!validId(newId) || find(newId)) { error = "new anonymous ID is invalid or exists"; return false; }
        profile.driverId = std::move(newId); profiles_.push_back(std::move(profile)); return true;
    }
    profile.revision = std::max(profile.revision, it->revision) + 1; *it = std::move(profile); return true;
}

std::vector<std::uint8_t> DriverProfileDatabase::serialize(std::string &error) const
{
    error.clear(); if (profiles_.size() > kMaximumDriverProfiles) {error="profile limit exceeded";return {};}
    std::vector<std::uint8_t> out(std::begin(magic), std::end(magic) - 1); put32(out, 1); put32(out, static_cast<std::uint32_t>(profiles_.size()));
    for (const auto &p : profiles_) {
        if (!validateProfile(p, error)) return {};
        putString(out,p.driverId); putString(out,p.displayName); put64(out,p.revision); put32(out,static_cast<std::uint32_t>(p.images.size()));
        for(const auto &i:p.images){out.push_back(static_cast<std::uint8_t>(i.source)); const auto *q=reinterpret_cast<const std::uint8_t*>(&i.quality);out.insert(out.end(),q,q+sizeof(float));put32(out,static_cast<std::uint32_t>(i.encodedImage.size()));out.insert(out.end(),i.encodedImage.begin(),i.encodedImage.end());}
        put32(out,static_cast<std::uint32_t>(p.embeddings.size()));
        for(const auto &e:p.embeddings){putString(out,e.modelId);put32(out,static_cast<std::uint32_t>(e.values.size()));const auto *v=reinterpret_cast<const std::uint8_t*>(e.values.data());out.insert(out.end(),v,v+e.values.size()*sizeof(float));}
    }
    return out;
}

std::optional<DriverProfileDatabase> DriverProfileDatabase::deserialize(const std::vector<std::uint8_t> &bytes, std::string &error)
{
    error.clear(); Reader r(bytes); char got[8]; std::uint32_t version,count;
    if(!r.take(got,8)||std::memcmp(got,magic,8)!=0||!r.u32(version)||version!=1||!r.u32(count)||count>kMaximumDriverProfiles){error="invalid profile payload header";return std::nullopt;}
    DriverProfileDatabase db;
    for(std::uint32_t n=0;n<count;++n){StoredDriverProfile p;std::uint32_t images,embeddings;
        if(!r.string(p.driverId)||!r.string(p.displayName)||!r.u64(p.revision)||!r.u32(images)||images>maximumImagesPerProfile){error="invalid profile payload";return std::nullopt;}
        for(std::uint32_t j=0;j<images;++j){std::uint8_t source;float quality;std::uint32_t size;if(!r.take(&source,1)||!r.take(&quality,sizeof quality)||!r.u32(size)||size==0||size>maximumEncodedImageBytes){error="invalid image payload";return std::nullopt;}EnrollmentImage i{static_cast<EnrollmentSource>(source),quality,{}};i.encodedImage.resize(size);if(!r.take(i.encodedImage.data(),size)){error="truncated image payload";return std::nullopt;}p.images.push_back(std::move(i));}
        if(!r.u32(embeddings)||embeddings>maximumEmbeddingsPerProfile){error="invalid embedding count";return std::nullopt;}
        for(std::uint32_t j=0;j<embeddings;++j){FaceEmbedding e;std::uint32_t dims;if(!r.string(e.modelId)||!r.u32(dims)||dims==0||dims>maximumEmbeddingDimensions){error="invalid embedding payload";return std::nullopt;}e.values.resize(dims);if(!r.take(e.values.data(),dims*sizeof(float))){error="truncated embedding payload";return std::nullopt;}p.embeddings.push_back(std::move(e));}
        if(!validateProfile(p,error)||db.find(p.driverId)){if(error.empty())error="duplicate driver ID";return std::nullopt;}db.profiles_.push_back(std::move(p));
    }
    if(!r.finished()){error="trailing profile payload data";return std::nullopt;} return db;
}
} // namespace dms
