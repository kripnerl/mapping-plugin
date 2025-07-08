#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
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

class PluginInterfaceFixture {
public:
    static IDAM_PLUGIN_INTERFACE* Create(
        const std::string& function = "get",
        const std::string& machine = "test_machine", 
        const std::string& path = "test_ids",
        int shot = 12345
    );

};

MapArguments makeMapArguments(IDAM_PLUGIN_INTERFACE* plugin_interface,
                              SignalType sig_type = SignalType::DEFAULT);