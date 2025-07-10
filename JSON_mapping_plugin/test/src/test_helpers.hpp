#pragma once

// UDA includes
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <clientserver/udaTypes.h>
#include <clientserver/udaStructs.h>

#include "map_types/map_arguments.hpp"

MapArguments makeMapArguments(DATA_BLOCK* datablock, UDA_TYPE datatype,
                              int rank, SignalType sig_type = SignalType::DEFAULT);
