#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <clientserver/udaStructs.h>
#include <nlohmann/json.hpp>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include "map_types/map_arguments.hpp"

// struct PluginInterfaceDeleter {
//     void operator()(IDAM_PLUGIN_INTERFACE* interface) const {
//         if (interface) {
//             udaFreePluginInterface(interface);
//         }
//     }
// };

class DataBlockFixture {
public:
    static DATA_BLOCK* Create(
        const std::string& function = "get",
        const std::string& machine = "test_machine", 
        const std::string& path = "test_ids",
        int shot = 12345
    );

};

MapArguments makeMapArguments(DATA_BLOCK* datablock, UDA_TYPE datatype,
                              int rank, SignalType sig_type = SignalType::DEFAULT);