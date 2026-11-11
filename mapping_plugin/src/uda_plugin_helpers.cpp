#include "uda_plugin_helpers.hpp"

#include <cstdlib>
#include <gsl/gsl-lite.hpp>
#include <libtokamap.hpp>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <clientserver/initStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

#include "array_order.hpp"

libtokamap::DataType mapping_plugin::uda_to_libtokamap_map(UDA_TYPE datatype) {

    auto libtokamap_type = libtokamap::DataType::Unknown;
    switch (datatype) {
        case UDA_TYPE_CHAR:
            libtokamap_type = libtokamap::DataType::Int8;
            break;
        case UDA_TYPE_SHORT:
            libtokamap_type = libtokamap::DataType::Int16;
            break;
        case UDA_TYPE_INT:
            libtokamap_type = libtokamap::DataType::Int32;
            break;
        case UDA_TYPE_UNSIGNED_INT:
            libtokamap_type = libtokamap::DataType::UInt32;
            break;
        case UDA_TYPE_LONG:
            libtokamap_type = libtokamap::DataType::Int64;
            break;
        case UDA_TYPE_FLOAT:
            libtokamap_type = libtokamap::DataType::Float;
            break;
        case UDA_TYPE_DOUBLE:
            libtokamap_type = libtokamap::DataType::Double;
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            libtokamap_type = libtokamap::DataType::UInt8;
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            libtokamap_type = libtokamap::DataType::UInt16;
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            libtokamap_type = libtokamap::DataType::UInt64;
            break;
        case UDA_TYPE_LONG64:
            libtokamap_type = libtokamap::DataType::Int64;
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            libtokamap_type = libtokamap::DataType::UInt64;
            break;
        default:
            break;
    }
    return libtokamap_type;
}

UDA_TYPE mapping_plugin::libtokamap_to_uda_map(libtokamap::DataType libtokamap_type) {

    auto uda_type = UDA_TYPE_UNKNOWN;
    switch (libtokamap_type) {
        case libtokamap::DataType::Int8:
            uda_type = UDA_TYPE_CHAR;
            break;
        case libtokamap::DataType::Int16:
            uda_type = UDA_TYPE_SHORT;
            break;
        case libtokamap::DataType::Int32:
            uda_type = UDA_TYPE_INT;
            break;
        case libtokamap::DataType::Int64:
            uda_type = UDA_TYPE_LONG64;
            break;
        case libtokamap::DataType::UInt8:
            uda_type = UDA_TYPE_UNSIGNED_CHAR;
            break;
        case libtokamap::DataType::UInt16:
            uda_type = UDA_TYPE_UNSIGNED_SHORT;
            break;
        case libtokamap::DataType::UInt32:
            uda_type = UDA_TYPE_UNSIGNED_INT;
            break;
        case libtokamap::DataType::UInt64:
            uda_type = UDA_TYPE_UNSIGNED_LONG64;
            break;
        case libtokamap::DataType::Float:
            uda_type = UDA_TYPE_FLOAT;
            break;
        case libtokamap::DataType::Double:
            uda_type = UDA_TYPE_DOUBLE;
            break;
        default:
            break;
    }
    return uda_type;
}

void mapping_plugin::set_data_block(DATA_BLOCK* data_block, libtokamap::TypedDataArray& array)
{
    initDataBlock(data_block);

    data_block->rank = array.rank();
    data_block->data_type = libtokamap_to_uda_map(array.data_type());
    data_block->data_n = static_cast<int>(array.size());

    if (array.rank() < 2) {
        data_block->data = array.release();
    } else {
        // libtokamap arrays are row-major, a UDA data block is read
        // column-major: see array_order.hpp. Without this the client gets the
        // right shape with the values in the wrong places.
        const size_t n_bytes = array.size() * libtokamap::data_type_size(array.data_type());
        auto* column_major = static_cast<char*>(malloc(n_bytes));
        to_column_major(array.buffer(), array.shape(), libtokamap::data_type_size(array.data_type()), column_major);
        data_block->data = column_major;
    }

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
