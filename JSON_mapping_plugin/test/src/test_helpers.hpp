#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <nlohmann/json.hpp>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
// #include <memory>

// struct PluginInterfaceDeleter {
//     void operator()(IDAM_PLUGIN_INTERFACE* interface) const {
//         if (interface) {
//             udaFreePluginInterface(interface);
//         }
//     }
// };

class PluginInterfaceFixture {
public:
    // using PluginInterfacePtr = std::unique_ptr<IDAM_PLUGIN_INTERFACE, PluginInterfaceDeleter>;
    //
    // static PluginInterfacePtr Create(
    //     const std::string& function = "get",
    //     const std::string& machine = "test_machine", 
    //     const std::string& path = "test_ids",
    //     int shot = 12345
    // );
    static IDAM_PLUGIN_INTERFACE* Create(
        const std::string& function = "get",
        const std::string& machine = "test_machine", 
        const std::string& path = "test_ids",
        int shot = 12345
    );

};

// Test-specific JSON mapping files
extern const nlohmann::json TEST_MAPPINGS_CFG;
extern const nlohmann::json TEST_GLOBALS;
extern const nlohmann::json TEST_MAPPINGS;
extern const nlohmann::json TEST_SHOT_GLOBALS;
extern const nlohmann::json TEST_SHOT_MAPPINGS;