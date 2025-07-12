#pragma once

// UDA includes
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <clientserver/udaTypes.h>
#include <clientserver/udaStructs.h>

#include "map_types/map_arguments.hpp"

json_mapping::MapArguments makeMapArguments(DATA_BLOCK* datablock, UDA_TYPE datatype,
                              int rank, json_mapping::SignalType sig_type = json_mapping::SignalType::DEFAULT);
