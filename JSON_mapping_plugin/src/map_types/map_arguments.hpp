#pragma once

#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <string_view>

#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

namespace json_mapping {

enum class SignalType : uint8_t { DEFAULT, DATA, TIME, ERROR, DIM, INVALID };

class Mapping;

struct MapArguments {
    const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries;
    const nlohmann::json& global_data;
    DATA_BLOCK* datablock;
    SignalType sig_type;
    UDA_TYPE datatype;
    int rank;

    explicit MapArguments(DATA_BLOCK* datablock,
                          const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const SignalType sig_type,
                          const UDA_TYPE datatype, const int rank)
            : entries{entries}
            , global_data{global_data}
            , datablock{datablock}
            , sig_type{sig_type}
            , datatype{datatype}
            , rank{rank}
    {}
};

/**
 * @brief Deduce the type of signal being requested/mapped,
 * currently using string comparisons
 *
 * The final_path_element for error can be either be error_upper
 * or error_lower so search for substring error.
 *
 * @param element_back_str requested IDS path suffix (eg. data, time, error).
 * @note if no string is supplied, SignalType set to invalid
 * @return SignalType Enum class containing the current signal type
 * [DEFAULT, INVALID, DATA, TIME, ERROR]
 */
SignalType deduce_signal_type(std::string_view final_path_element);

} // namespace json_mapping
