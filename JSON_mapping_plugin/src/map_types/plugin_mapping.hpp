#pragma once

#include "base_mapping.hpp"
#include "utils/ram_cache.hpp"
#include "map_types/map_arguments.hpp"

#include <optional>
#include <unordered_map>
#include <utility>
#include <string>
#include <memory>

#include <plugins/pluginStructs.h>
#include <clientserver/udaStructs.h>

namespace json_mapping {

using MapArgs_t = std::unordered_map<std::string, nlohmann::json>;

class PluginMapping : public Mapping
{
  public:
    PluginMapping() = delete;
    PluginMapping(std::string plugin, MapArgs_t request_args, std::optional<float> offset, std::optional<float> scale,
                  std::optional<std::string> slice, std::optional<std::string> function,
                  std::shared_ptr<ram_cache::RamCache> ram_cache, const PLUGINLIST* plugin_list)
        : m_plugin{std::move(plugin)}
        , m_map_args{std::move(request_args)}
        , m_offset{offset}
        , m_scale{scale}
        , m_slice{std::move(slice)}
        , m_function{std::move(function)}
        , m_ram_cache{std::move(ram_cache)}
        , m_cache_enabled(m_ram_cache != nullptr)
        , m_plugin_list{plugin_list}
        {};

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_plugin;
    MapArgs_t m_map_args;
    std::optional<float> m_offset;
    std::optional<float> m_scale;
    std::optional<std::string> m_slice;
    std::optional<std::string> m_function;
    std::shared_ptr<ram_cache::RamCache> m_ram_cache;
    bool m_cache_enabled;
    const PLUGINLIST* m_plugin_list;

    [[nodiscard]] std::string get_request_str(const MapArguments& arguments) const;
    [[nodiscard]] bool copy_from_cache(DATA_BLOCK* data_block, const MapArguments& arguments, const std::string& request_str) const;
    [[nodiscard]] int call_plugins(DATA_BLOCK* data_block, const MapArguments& arguments) const;
};

} // namespace json_mapping
