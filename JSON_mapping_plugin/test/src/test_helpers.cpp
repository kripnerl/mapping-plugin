#include "test_helpers.hpp"
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>
#include <unordered_map>
#include "map_types/base_mapping.hpp"
#include <memory>

IDAM_PLUGIN_INTERFACE* PluginInterfaceFixture::Create(
    const std::string& function,
    const std::string& machine,
    const std::string& path,
    int shot
) {
    std::string request = function + "::" + machine + "/" + path + "?shot=" + std::to_string(shot);

    IDAM_PLUGIN_INTERFACE* interface = udaCreatePluginInterface(request.c_str()); // interface
    interface->data_block = static_cast<DATA_BLOCK*>(malloc(sizeof(DATA_BLOCK))); // allocate datablock

    return interface;
}

MapArguments makeMapArguments(IDAM_PLUGIN_INTERFACE* plugin_interface,
                              SignalType sig_type) {
    static std::unordered_map<std::string, std::unique_ptr<Mapping>> empty_entries;
    static nlohmann::json empty_global_data = nlohmann::json::object();

    return MapArguments(plugin_interface, empty_entries, empty_global_data, sig_type);
}