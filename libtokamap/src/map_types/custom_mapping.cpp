#include "map_types/custom_mapping.hpp"

#include <cstring>

#include "map_types/map_arguments.hpp"

/**
 * @brief Entry map function, overriden from parent Mapping class
 *
 * @note expression is only of float type for testing purposes
 * @param interface IDAM_PLUGIN_INTERFACE for access to request and data_block
 * @param entries unordered map of all mappings loaded for this experiment and
 * group
 * @param global_data global JSON object used in templating
 * @return int error_code
 */
libtokamap::TypedDataArray libtokamap::CustomMapping::map(const MapArguments& /*arguments*/) const
{
    switch (m_custom_type) {
        case CustomMapType_t::MASTU_helloworld:
            return MASTU_helloworld();
            break;
        case CustomMapType_t::DRAFT_helloworld:
            return DRAFT_helloworld();
            break;
        case CustomMapType_t::INVALID:
            return {};
            break;
    }
    return {};
}

libtokamap::TypedDataArray libtokamap::CustomMapping::MASTU_helloworld()
{
    const char* string = "Hello World from MASTU";
    return TypedDataArray{ string };
}

libtokamap::TypedDataArray libtokamap::CustomMapping::DRAFT_helloworld()
{
    const char* string = "Hello World from DRAFT";
    return TypedDataArray{ string };
}
