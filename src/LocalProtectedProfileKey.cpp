#include "LocalProtectedProfileKey.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <dpapi.h>
#endif

namespace dms
{
namespace
{
#ifdef _WIN32
constexpr std::size_t secretBytes = 32;
constexpr char hex[] = "0123456789abcdef";

std::string encodeSecret(const std::uint8_t *bytes)
{
    std::string value(secretBytes * 2, '\0');
    for (std::size_t i = 0; i < secretBytes; ++i)
    {
        value[i * 2] = hex[bytes[i] >> 4];
        value[i * 2 + 1] = hex[bytes[i] & 0x0f];
    }
    return value;
}
#endif
} // namespace

bool createLocalProtectedProfileKey(std::vector<std::uint8_t> &protectedBlob,
                                    std::string &secret, std::string &error) noexcept
{
    protectedBlob.clear(); secret.clear(); error.clear();
#ifndef _WIN32
    error = "local protected profile keys are not configured on this platform";
    return false;
#else
    std::uint8_t raw[secretBytes]{};
    if (BCryptGenRandom(nullptr, raw, sizeof raw, BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
    {
        error = "secure local profile key generation failed";
        return false;
    }
    DATA_BLOB input{static_cast<DWORD>(sizeof raw), raw}; DATA_BLOB output{};
    const BOOL ok = CryptProtectData(&input, L"DMS Next local driver profiles", nullptr, nullptr,
                                     nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output);
    if (ok)
    {
        protectedBlob.assign(output.pbData, output.pbData + output.cbData);
        secret = encodeSecret(raw);
        LocalFree(output.pbData);
    }
    SecureZeroMemory(raw, sizeof raw);
    if (!ok)
    {
        error = "operating system profile-key protection failed";
        return false;
    }
    return true;
#endif
}

bool openLocalProtectedProfileKey(const std::vector<std::uint8_t> &protectedBlob,
                                  std::string &secret, std::string &error) noexcept
{
    secret.clear(); error.clear();
#ifndef _WIN32
    (void)protectedBlob;
    error = "local protected profile keys are not configured on this platform";
    return false;
#else
    if (protectedBlob.empty() || protectedBlob.size() > 64 * 1024)
    {
        error = "invalid protected profile key";
        return false;
    }
    DATA_BLOB input{static_cast<DWORD>(protectedBlob.size()),
                    const_cast<BYTE *>(protectedBlob.data())};
    DATA_BLOB output{};
    const BOOL ok = CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                                       CRYPTPROTECT_UI_FORBIDDEN, &output);
    if (!ok || output.cbData != secretBytes)
    {
        if (output.pbData) LocalFree(output.pbData);
        error = "protected profile key is invalid or unavailable to this OS user";
        return false;
    }
    secret = encodeSecret(output.pbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return true;
#endif
}
} // namespace dms
