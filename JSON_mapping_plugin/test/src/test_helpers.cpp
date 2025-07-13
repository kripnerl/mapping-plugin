#include "test_helpers.hpp"

#include <unordered_map>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

// UDA includes
#include <clientserver/udaTypes.h>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

json_mapping::MapArguments makeMapArguments(const UDA_TYPE datatype, const int rank, const json_mapping::SignalType sig_type) {
    static std::unordered_map<std::string, std::unique_ptr<json_mapping::Mapping>> empty_entries;
    static nlohmann::json empty_global_data = nlohmann::json::object();

    return json_mapping::MapArguments(empty_entries, empty_global_data, sig_type, datatype, rank);
}
