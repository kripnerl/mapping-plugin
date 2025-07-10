#include "test_helpers.hpp"

#include <unordered_map>
#include <memory>
#include <string>

// UDA includes
#include <clientserver/udaTypes.h>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>

#include "map_types/base_mapping.hpp"

MapArguments makeMapArguments(DATA_BLOCK* datablock, const UDA_TYPE datatype,
                              const int rank, const SignalType sig_type) {
    static std::unordered_map<std::string, std::unique_ptr<Mapping>> empty_entries;
    static nlohmann::json empty_global_data = nlohmann::json::object();

    return MapArguments(datablock, empty_entries, empty_global_data, sig_type, datatype, rank);
}
