#include "map_types/custom_mapping.hpp"

// UDA includes
#include <clientserver/udaStructs.h>
#include <plugins/udaPlugin.h>

#include "map_types/map_arguments.hpp"

/**
 * @brief Entry map function, overriden from parent Mapping class
 *
 * @note expression is only of float type for testing purposes
 * @param interface IDAM_PLUGIN_INTERFACE for access to request and data_block
 * @param entries unordered map of all mappings loaded for this experiment and
 * IDS
 * @param global_data global JSON object used in templating
 * @return int error_code
 */
int json_mapping::CustomMapping::map(const MapArguments& arguments) const
{
    int err{1};
    switch (m_custom_type) {
        case CustomMapType_t::MASTU_helloworld:
            err = MASTU_helloworld(arguments.datablock);
            break;
        case CustomMapType_t::DRAFT_helloworld:
            err = DRAFT_helloworld(arguments.datablock);
            break;
        case CustomMapType_t::INVALID:
            break;
    }

    return err;
}

int json_mapping::CustomMapping::MASTU_helloworld(DATA_BLOCK* data_block)
{
    return setReturnDataString(data_block, "Hello World from MASTU", nullptr);
}

int json_mapping::CustomMapping::DRAFT_helloworld(DATA_BLOCK* data_block)
{
    return setReturnDataString(data_block, "Hello World from DRAFT", nullptr);
}
