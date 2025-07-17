#include "test_helpers.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

json_mapping::MapArguments makeMapArguments(const std::type_index data_type, const int rank,
                                            const json_mapping::SignalType sig_type)
{
    static std::unordered_map<std::string, std::unique_ptr<json_mapping::Mapping>> empty_entries;
    static nlohmann::json empty_global_data = nlohmann::json::object();

    return json_mapping::MapArguments(empty_entries, empty_global_data, sig_type, data_type, rank);
}
