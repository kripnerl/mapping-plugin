#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "utils/types.hpp"

namespace libtokamap
{

nlohmann::json find_partition_attributes(const PartitionList& partitions, const nlohmann::json& attributes);

std::filesystem::path find_partition_directory(std::filesystem::path mapping_dir, const PartitionList& partitions,
                                               const nlohmann::json& attributes);

namespace detail
{
std::string select_subdirectory(const std::vector<std::string>& subdirectories, DirectorySelector selector,
                                const std::string& value);
std::string select_subdirectory(const std::vector<std::string>& subdirectories, DirectorySelector selector, int value);
} // namespace detail

} // namespace libtokamap
