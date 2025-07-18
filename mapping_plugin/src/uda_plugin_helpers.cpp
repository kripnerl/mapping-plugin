#include "uda_plugin_helpers.hpp"

#include <cstring>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <string>
#include <gsl/gsl-lite.hpp>

#include <clientserver/initStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

#include "map_types/map_arguments.hpp"

std::unordered_map<std::string, UDA_TYPE> json_plugin::uda_type_map()
{
    static std::unordered_map<std::string, UDA_TYPE> type_map;
    if (type_map.empty()) {
        type_map = {{typeid(unsigned int).name(), UDA_TYPE_UNSIGNED_INT},
                    {typeid(int).name(), UDA_TYPE_INT},
                    {typeid(float).name(), UDA_TYPE_FLOAT},
                    {typeid(double).name(), UDA_TYPE_DOUBLE}};
    }
    return type_map;
}

std::unordered_map<std::type_index, UDA_TYPE> json_plugin::uda_type_index_map()
{
    static std::unordered_map<std::type_index, UDA_TYPE> type_map;
    if (type_map.empty()) {
        type_map = {{std::type_index{ typeid(unsigned int) }, UDA_TYPE_UNSIGNED_INT},
                    {std::type_index{ typeid(int) }, UDA_TYPE_INT},
                    {std::type_index{ typeid(float) }, UDA_TYPE_FLOAT},
                    {std::type_index{ typeid(double) }, UDA_TYPE_DOUBLE}};
    }
    return type_map;
}

void json_plugin::set_data_block(DATA_BLOCK* data_block, const libtokamap::TypedDataArray& array)
{
    initDataBlock(data_block);

    data_block->rank = array.rank();
    data_block->data_type = uda_type_index_map().at(array.type_index());
    data_block->data = const_cast<char*>(array.buffer());
    data_block->data_n = static_cast<int>(array.size());

    data_block->dims = static_cast<DIMS*>(malloc(data_block->rank * sizeof(DIMS)));

    auto dims = gsl::span(data_block->dims, data_block->rank);

    size_t len = 1;

    for (size_t i = 0; i < data_block->rank; ++i) {
        initDimBlock(&dims[i]);

        int const shape_i = static_cast<int>(array.shape()[i]);
        dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        dims[i].dim_n = shape_i;

        // Always setting the dim to compressed initial value and spacing
        dims[i].compressed = 1;
        dims[i].dim0 = 0.0;
        dims[i].diff = 1.0;
        dims[i].method = 0;

        len *= shape_i;
    }
}
