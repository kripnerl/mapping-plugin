#include "map_types/dim_mapping.hpp"

#include <cstdlib>

// UDA includes
#include <clientserver/udaStructs.h>
#include <plugins/udaPlugin.h>
#include <logging/logging.h>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

int DimMapping::map(const MapArguments& arguments) const
{
    if (arguments.entries.count(m_dim_probe) == 0) {
        return 1;
    }

    int err = arguments.entries.at(m_dim_probe)->map(arguments);
    if (err == 0) {
        free((void*)arguments.datablock->data); // fix
        arguments.datablock->data = nullptr;
        if (arguments.datablock->data_n == 0) {
            UDA_LOG(UDA_LOG_DEBUG, "\nDimMapping::map: Dim probe could not be used for Shape_of \n");
            return 1;
        }
        err = setReturnDataIntScalar(arguments.datablock, arguments.datablock->data_n,
                                     nullptr);
    }
    return err;
}
