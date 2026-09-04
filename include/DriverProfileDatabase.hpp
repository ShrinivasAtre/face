#pragma once

#include "DriverIdentity.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dms
{
enum class EnrollmentSource : std::uint8_t { Photo = 1, Video = 2, Live = 3, Automatic = 4 };

struct EnrollmentImage
{
    EnrollmentSource source = EnrollmentSource::Photo;
    float quality = 0.0F;
    std::vector<std::uint8_t> encodedImage;
};

struct StoredDriverProfile
{
    std::string driverId;
    std::string displayName;
    std::uint64_t revision = 1;
    std::vector<EnrollmentImage> images;
    std::vector<FaceEmbedding> embeddings;
};

enum class ImportConflict { Reject, Replace, NewAnonymousId };

class DriverProfileDatabase
{
  public:
    static constexpr std::size_t maximumImagesPerProfile = 10;
    static constexpr std::size_t maximumEmbeddingsPerProfile = 10;
    static constexpr std::size_t maximumEncodedImageBytes = 16 * 1024 * 1024;

    const std::vector<StoredDriverProfile> &profiles() const noexcept { return profiles_; }
    const StoredDriverProfile *find(const std::string &driverId) const noexcept;
    bool create(std::string driverId, std::string displayName, std::string &error);
    bool erase(const std::string &driverId, std::string &error);
    bool addImage(const std::string &driverId, EnrollmentImage image, std::string &error);
    bool addEmbedding(const std::string &driverId, FaceEmbedding embedding, std::string &error);
    bool importProfile(StoredDriverProfile profile, ImportConflict conflict, std::string newAnonymousId,
                       std::string &error);
    std::vector<std::uint8_t> serialize(std::string &error) const;
    static std::optional<DriverProfileDatabase> deserialize(const std::vector<std::uint8_t> &bytes,
                                                             std::string &error);

  private:
    std::vector<StoredDriverProfile> profiles_;
};
} // namespace dms
