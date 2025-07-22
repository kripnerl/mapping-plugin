#include "data_source_mapping.hpp"

#include <inja/inja.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"
#include "utils/subset.hpp"

std::unordered_map<std::string, std::unique_ptr<libtokamap::DataSource>> libtokamap::DataSourceMapping::m_data_sources =
    {};

libtokamap::TypedDataArray libtokamap::DataSourceMapping::map(const MapArguments& arguments) const
{
    DataSourceArgs args = m_data_source_args;
    for (auto& [key, value] : args) {
        if (value.is_string()) {
            value = inja::render(inja::render(value.get<std::string>(), arguments.global_data), arguments.global_data);
        }
    }
    TypedDataArray array = m_data_source->get(args, arguments, m_ram_cache.get());
    update_array(array, m_slice, m_scale, m_offset);
    return array;
}

libtokamap::DataSourceMapping::DataSourceMapping(const std::string& data_source_name, DataSourceArgs data_source_args,
                                                 std::optional<float> offset, std::optional<float> scale,
                                                 std::optional<std::string> slice,
                                                 std::shared_ptr<libtokamap::RamCache> ram_cache)
    : m_data_source_args{std::move(data_source_args)}, m_offset{offset}, m_scale{scale}, m_slice{std::move(slice)},
      m_ram_cache{std::move(ram_cache)}, m_cache_enabled(m_ram_cache != nullptr)
{
    if (!m_data_sources.contains(data_source_name)) {
        throw libtokamap::DataSourceError{"data source " + data_source_name + " not registered"};
    }
    m_data_source = m_data_sources[data_source_name].get();
}
