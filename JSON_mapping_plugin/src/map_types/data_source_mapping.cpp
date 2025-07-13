#include "data_source_mapping.hpp"

#include <unordered_map>
#include <string>
#include <memory>

#include "map_types/map_arguments.hpp"

std::unordered_map<std::string, std::unique_ptr<json_mapping::DataSource>> json_mapping::DataSourceMapping::m_data_sources = {};

json_mapping::TypedDataArray json_mapping::DataSourceMapping::map(const MapArguments& arguments) const
{
    return m_data_source->get(m_data_source_args, arguments, m_ram_cache.get(), m_scale, m_offset, m_slice);
}
