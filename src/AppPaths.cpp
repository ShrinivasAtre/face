#include "AppPaths.hpp"

#ifdef _WIN32

#include <windows.h>

#else

#include <unistd.h>

#endif

#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;


fs::path AppPaths::executableDirectory()
{
#ifdef _WIN32

    wchar_t buffer[32768];

    DWORD length =
        GetModuleFileNameW(
            nullptr,
            buffer,
            static_cast<DWORD>(std::size(buffer))
        );

    if (length == 0)
    {
        throw std::runtime_error(
            "GetModuleFileNameW failed"
        );
    }

    return fs::path(
        std::wstring(buffer, length)
    ).parent_path();

#else

    char buffer[4096];

    ssize_t length =
        readlink(
            "/proc/self/exe",
            buffer,
            sizeof(buffer) - 1
        );

    if (length <= 0)
    {
        throw std::runtime_error(
            "Failed to determine executable path"
        );
    }

    buffer[length] = '\0';

    return fs::path(buffer).parent_path();

#endif
}