#pragma once

#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

#include <clientserver/udaStructs.h>
#include <plugins/pluginStructs.h>

#include "map_types/data_source_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

class UDADataSource : public json_mapping::DataSource
{
public:
    explicit UDADataSource(std::string plugin_name, std::optional<std::string> function, PluginList* plugin_list, bool cache_enabled)
        : m_plugin_name{ std::move(plugin_name) }
        , m_function{ std::move(function) }
        , m_plugin_list{ plugin_list }
        , m_cache_enabled{ cache_enabled }
    {}
    json_mapping::TypedDataArray get(const json_mapping::DataSourceArgs& data_source_args, const json_mapping::MapArguments& arguments, ram_cache::RamCache* ram_cache, std::optional<float> scale, std::optional<float> offset, std::optional<std::string> slice) override;

private:
    std::string m_plugin_name;
    std::optional<std::string> m_function;
    PluginList* m_plugin_list;
    bool m_cache_enabled;

    [[nodiscard]] std::string get_request_str(const json_mapping::DataSourceArgs& data_source_args, const json_mapping::MapArguments& arguments, std::optional<std::string> slice) const;
    [[nodiscard]] bool copy_from_cache(ram_cache::RamCache* ram_cache, DATA_BLOCK* data_block, const json_mapping::MapArguments& arguments,
                                       const std::string& request_str) const;
    [[nodiscard]] int call_plugins(DATA_BLOCK* data_block, const json_mapping::DataSourceArgs& data_source_args, const json_mapping::MapArguments& arguments, ram_cache::RamCache* ram_cache, std::optional<float> scale, std::optional<float> offset, std::optional<std::string> slice) const;
};
