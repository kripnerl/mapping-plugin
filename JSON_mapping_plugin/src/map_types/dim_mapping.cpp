#include "map_types/dim_mapping.hpp"

#include <cstdlib>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

json_mapping::TypedDataArray json_mapping::DimMapping::map(const MapArguments& arguments) const
{
    if (arguments.entries.count(m_dim_probe) == 0) {
        return {};
    }

    auto array = arguments.entries.at(m_dim_probe)->map(arguments);
    if (!array.empty()) {
        return TypedDataArray{ array.size() };
    }

    return {};
}
