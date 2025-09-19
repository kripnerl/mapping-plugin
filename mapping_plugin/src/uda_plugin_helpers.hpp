#pragma once

#include <cstring>
#include <libtokamap.hpp>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

namespace json_plugin
{

std::unordered_map<std::string, UDA_TYPE> uda_type_map();
std::unordered_map<std::type_index, UDA_TYPE> uda_type_index_map();
void set_data_block(DATA_BLOCK* data_block, libtokamap::TypedDataArray& array);

} // namespace json_plugin
