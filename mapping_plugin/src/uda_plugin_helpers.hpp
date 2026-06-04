#pragma once

#include <cstring>
#include <libtokamap.hpp>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

namespace mapping_plugin
{

libtokamap::DataType uda_to_libtokamap_map(UDA_TYPE datatype);
UDA_TYPE libtokamap_to_uda_map(libtokamap::DataType datatype);
void set_data_block(DATA_BLOCK* data_block, libtokamap::TypedDataArray& array);

} // namespace mapping_plugin
