#include "map_types/dim_mapping.hpp"

#include <cstdlib>

#include "exceptions/exceptions.hpp"
#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

libtokamap::TypedDataArray libtokamap::DimMapping::map(const MapArguments& arguments) const
{
    if (!arguments.entries.contains(m_dim_probe)) {
        throw libtokamap::MappingError{"invalid DIM_PROBE '" + m_dim_probe + "'"};
    }

    auto array = arguments.entries.at(m_dim_probe)->map(arguments);
    return TypedDataArray{array.size()};
}
