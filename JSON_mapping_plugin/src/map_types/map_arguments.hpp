#pragma once

#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include <cstdint>

#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

enum class SignalType : uint8_t { DEFAULT, DATA, TIME, ERROR, DIM, INVALID };

class Mapping;

struct MapArguments {
    const std::unordered_map<std::string, std::unique_ptr<Mapping>>& m_entries;
    const nlohmann::json& m_global_data;
    DATA_BLOCK* m_datablock;
    SignalType m_sig_type;
    UDA_TYPE m_datatype;
    int m_rank;

    explicit MapArguments(DATA_BLOCK* datablock,
                          const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const SignalType sig_type,
                          const UDA_TYPE datatype, const int rank)
            : m_entries{entries}
            , m_global_data{global_data}
            , m_datablock{datablock}
            , m_sig_type{sig_type}
            , m_datatype{datatype}
            , m_rank{rank}
    {}

};
