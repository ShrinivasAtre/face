#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dms
{
// Creates a random profile-store secret and protects it with the current OS
// user account. The returned secret is suitable only as an in-memory input to
// the profile-bundle encryption layer; callers persist protectedBlob instead.
bool createLocalProtectedProfileKey(std::vector<std::uint8_t> &protectedBlob,
                                    std::string &secret, std::string &error) noexcept;

// Recovers a profile-store secret for the current OS user. Modified blobs and
// blobs belonging to another user or machine fail closed.
bool openLocalProtectedProfileKey(const std::vector<std::uint8_t> &protectedBlob,
                                  std::string &secret, std::string &error) noexcept;
} // namespace dms
