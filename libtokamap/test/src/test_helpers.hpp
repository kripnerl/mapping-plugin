#pragma once

#include <typeindex>

#include "map_types/map_arguments.hpp"

libtokamap::MapArguments makeMapArguments(std::type_index data_type, int rank,
    libtokamap::SignalType sig_type = libtokamap::SignalType::DEFAULT);
