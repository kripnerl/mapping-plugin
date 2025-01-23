#pragma once

#include <clientserver/udaTypes.h>

enum class SignalType { DEFAULT, DATA, TIME, ERROR, DIM, INVALID };

class Mapping;

struct MapArguments {
    const std::unordered_map<std::string, std::unique_ptr<Mapping>>& m_entries;
    const nlohmann::json& m_global_data;
    IDAM_PLUGIN_INTERFACE* m_interface;
    // std::vector<int> m_indices;
    SignalType m_sig_type;
    UDA_TYPE m_datatype;
    int m_rank;

    explicit MapArguments(IDAM_PLUGIN_INTERFACE* interface,
                          const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const SignalType sig_type)
            : m_entries{entries}
            , m_global_data{global_data}
            , m_interface{interface}
            , m_sig_type{sig_type}
            , m_datatype{UDA_TYPE_UNKNOWN}
            , m_rank{-1}
    {
        extract_interface_arguments(interface);
    }

private:
    int extract_interface_arguments(const IDAM_PLUGIN_INTERFACE* interface)
    {
        const NAMEVALUELIST& nv_list = interface->request_data->nameValueList;

        // extract values from interface
        int datatype = UDA_TYPE_UNKNOWN;
        FIND_REQUIRED_INT_VALUE(nv_list, datatype);
        if (datatype < 0 || datatype > UDA_TYPE_CAPNP) {
            RAISE_PLUGIN_ERROR("Invalid datatype");
        }

        int rank;
        FIND_REQUIRED_INT_VALUE(nv_list, rank);
        if (rank < 0) {
            RAISE_PLUGIN_ERROR("Invalid rank");
        }

        m_datatype = static_cast<UDA_TYPE>(datatype);
        m_rank = rank;

        return 0;
    }
};
