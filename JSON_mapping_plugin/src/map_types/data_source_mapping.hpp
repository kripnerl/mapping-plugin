#pragma once

#include "base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <clientserver/udaStructs.h>
#include <plugins/pluginStructs.h>

namespace json_mapping
{

using DataSourceArgs = std::unordered_map<std::string, nlohmann::json>;

class DataSource {
public:
    DataSource() = default;
    virtual TypedDataArray get(const DataSourceArgs& map_args, const MapArguments& arguments, ram_cache::RamCache* ram_cache, std::optional<float> scale, std::optional<float> offset, std::optional<std::string> slice) = 0;
    virtual ~DataSource() = default;

    DataSource(DataSource&& other) = default;
    DataSource(const DataSource& other) = default;
    DataSource& operator=(DataSource&& other) = default;
    DataSource& operator=(const DataSource& other) = default;
};

class DataSourceMapping : public Mapping
{
  public:
    DataSourceMapping() = delete;
    DataSourceMapping(const std::string& data_source_name, DataSourceArgs data_source_args, std::optional<float> offset,
                      std::optional<float> scale, std::optional<std::string> slice, std::shared_ptr<ram_cache::RamCache> ram_cache)
        : m_data_source_args{std::move(data_source_args)}
        , m_offset{offset}
        , m_scale{scale}
        , m_slice{std::move(slice)}
        , m_ram_cache{std::move(ram_cache)}
        , m_cache_enabled(m_ram_cache != nullptr)
    {
        if (m_data_sources.count(data_source_name) == 0) {
            throw std::runtime_error{ "data source " + data_source_name + " not registered" };
        }
        m_data_source = m_data_sources[data_source_name].get();
    }

    static void register_data_source(const std::string& name, std::unique_ptr<DataSource> data_source)
    {
        m_data_sources[name] = std::move(data_source);
    }

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    static std::unordered_map<std::string, std::unique_ptr<DataSource>> m_data_sources;

    DataSource* m_data_source;
    DataSourceArgs m_data_source_args;
    std::optional<float> m_offset;
    std::optional<float> m_scale;
    std::optional<std::string> m_slice;
    std::shared_ptr<ram_cache::RamCache> m_ram_cache;
    bool m_cache_enabled;
};

} // namespace json_mapping
