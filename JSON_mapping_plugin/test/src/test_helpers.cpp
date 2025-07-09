#include "test_helpers.hpp"
#include <clientserver/udaTypes.h>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>
#include <unordered_map>
#include "map_types/base_mapping.hpp"
#include <memory>

DATA_BLOCK* DataBlockFixture::Create(
    const std::string& function,
    const std::string& machine,
    const std::string& path,
    int shot
) {
    std::string request = function + "::" + machine + "/" + path + "?shot=" + std::to_string(shot);

    // IDAM_PLUGIN_INTERFACE* interface = udaCreatePluginInterface(request.c_str()); // interface
    return static_cast<DATA_BLOCK*>(malloc(sizeof(DATA_BLOCK))); // allocate datablock

    // return interface;
}

MapArguments makeMapArguments(DATA_BLOCK* datablock, const UDA_TYPE datatype,
                              const int rank, const SignalType sig_type) {
    static std::unordered_map<std::string, std::unique_ptr<Mapping>> empty_entries;
    static nlohmann::json empty_global_data = nlohmann::json::object();

    return MapArguments(datablock, empty_entries, empty_global_data, sig_type, datatype, rank);
}