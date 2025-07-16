#pragma once

#include <typeindex>

#include "map_types/map_arguments.hpp"

json_mapping::MapArguments makeMapArguments(std::type_index data_type, int rank,
    json_mapping::SignalType sig_type = json_mapping::SignalType::DEFAULT);
