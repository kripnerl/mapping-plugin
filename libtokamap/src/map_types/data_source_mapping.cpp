#include "data_source_mapping.hpp"

#include <unordered_map>
#include <string>
#include <memory>

#include "map_types/map_arguments.hpp"
#include "utils/subset.hpp"

std::unordered_map<std::string, std::unique_ptr<libtokamap::DataSource>> libtokamap::DataSourceMapping::m_data_sources = {};

libtokamap::TypedDataArray libtokamap::DataSourceMapping::map(const MapArguments& arguments) const
{
    TypedDataArray array = m_data_source->get(m_data_source_args, arguments, m_ram_cache.get());
    subset::update_array(array, m_slice, m_scale, m_offset);
    return array;
}
