#pragma once

#include "DmsPresentationConfig.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace dms
{
class DmsPresentationConfigLoader
{
public:
    static std::optional<DmsPresentationConfig> load(
        const std::filesystem::path &path, std::string &error) noexcept;
};
}
