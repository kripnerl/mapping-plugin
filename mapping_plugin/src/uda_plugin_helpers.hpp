#pragma once

#include <cstring>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

#include "map_types/map_arguments.hpp"

namespace json_plugin
{

std::unordered_map<std::string, UDA_TYPE> uda_type_map();
std::unordered_map<std::type_index, UDA_TYPE> uda_type_index_map();
void set_data_block(DATA_BLOCK* data_block, libtokamap::TypedDataArray& array);

} // namespace json_plugin
