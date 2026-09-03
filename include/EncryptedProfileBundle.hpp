#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dms
{
struct BundleCryptoConfig
{
    std::uint32_t pbkdf2Iterations = 600000;
    bool validate(std::string &error) const noexcept;
};

bool encryptProfileBundle(const std::vector<std::uint8_t> &plaintext, const std::string &passphrase,
                          const BundleCryptoConfig &config, std::vector<std::uint8_t> &bundle,
                          std::string &error) noexcept;
bool decryptProfileBundle(const std::vector<std::uint8_t> &bundle, const std::string &passphrase,
                          std::vector<std::uint8_t> &plaintext, std::string &error) noexcept;
} // namespace dms
