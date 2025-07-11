#pragma once

#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>

#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

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
