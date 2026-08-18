#pragma once

#include <filesystem>

class AppPaths
{
public:

    static std::filesystem::path executableDirectory();
};