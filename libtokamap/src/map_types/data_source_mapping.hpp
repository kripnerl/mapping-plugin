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

namespace libtokamap
{

using DataSourceArgs = std::unordered_map<std::string, nlohmann::json>;

class DataSource
{
  public:
    DataSource() = default;
    virtual TypedDataArray get(const DataSourceArgs& map_args, const MapArguments& arguments, RamCache* ram_cache) = 0;
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
                      std::optional<float> scale, std::optional<std::string> slice,
                      std::shared_ptr<libtokamap::RamCache> ram_cache);

    static void register_data_source(const std::string& name, std::unique_ptr<DataSource> data_source)
    {
        m_data_sources[name] = std::move(data_source);
    }

    static void unregister_data_source(const std::string& name) { m_data_sources.erase(name); }

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    static std::unordered_map<std::string, std::unique_ptr<DataSource>> m_data_sources;

    DataSource* m_data_source;
    DataSourceArgs m_data_source_args;
    std::optional<float> m_offset;
    std::optional<float> m_scale;
    std::optional<std::string> m_slice;
    std::shared_ptr<libtokamap::RamCache> m_ram_cache;
    bool m_cache_enabled;
};

} // namespace libtokamap
