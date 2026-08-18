#include "AppPaths.hpp"

#ifdef _WIN32

#include <windows.h>

namespace AppPaths {

std::filesystem::path executableDirectory()
{
    wchar_t buffer[MAX_PATH];

    DWORD length = GetModuleFileNameW(
        nullptr,
        buffer,
        MAX_PATH
    );

    if (length == 0) {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(
        std::wstring(buffer, length)
    ).parent_path();
}

}

#else

#include <unistd.h>
#include <limits.h>

namespace AppPaths {

std::filesystem::path executableDirectory()
{
    char buffer[PATH_MAX];

    ssize_t length =
        readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

    if (length <= 0) {
        return std::filesystem::current_path();
    }

    buffer[length] = '\0';

    return std::filesystem::path(buffer).parent_path();
}

}

#endif
