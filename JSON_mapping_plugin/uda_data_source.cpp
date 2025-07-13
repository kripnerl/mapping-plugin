#include "uda_data_source.hpp"

#include <cstddef>
#include <cstring>
#include <map_types/data_source_mapping.hpp>
#include <stdexcept>
#include <exception>
#include <fmt/core.h>
#include <inja/inja.hpp>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <utility>

// UDA includes
#include <client/getEnvironment.h>
#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/parseXML.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <structures/struct.h>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>

#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"
#include "utils/scale_offset.hpp"
#include "utils/subset.hpp"

// TODO:
//  - handle compressed dims
//  - handle error arrays (how to determine not empty?)
//  - only read required data out of cache for each request (i.e. data, error, or a single dim)

/**
 * @brief
 *
 * eg. UDA::get(signal=/AMC/ROGEXT/P1U, source=45460, host=uda2.hpc.l, port=56565)
 * eg. GEOM::get(signal=/magnetics/pfcoil/d1_upper, Config=1);
 * eg. JSONDataReader::get(signal=/APC/plasma_current);
 *
 * @param json_globals
 * @return
 */
std::string UDADataSource::get_request_str(const json_mapping::DataSourceArgs& data_source_args, const json_mapping::MapArguments& arguments, std::optional<std::string> slice) const
{
    std::stringstream string_stream;
    string_stream << m_plugin_name << "::" << m_function.value_or("get") << "(";

    // m_map_args 'field' currently nlohmann json
    // parse to string/bool
    // TODO: change, however std::any/std::variant functionality for free
    const char* delim = "";
    for (const auto& [key, field] : data_source_args) {
        if (field.is_string()) {
            // Double inja
            try {
                auto value =
                    inja::render(inja::render(field.get<std::string>(), arguments.global_data), arguments.global_data);
                string_stream << delim << key << "=" << value;
            } catch (std::exception& e) {
                // UDA_LOG(UDA_LOG_DEBUG, "Inja template error in request : %s\n", e.what());
                return {};
            }
        } else if (field.is_boolean()) {
            string_stream << delim << key;
        } else {
            continue;
        }
        delim = ", ";
    }
    string_stream << ")";

    if (slice.has_value() && arguments.sig_type != json_mapping::SignalType::DIM) {
        string_stream << inja::render(inja::render(slice.value(), arguments.global_data), arguments.global_data);
    }

    auto request = string_stream.str();
    // UDA_LOG(UDA_LOG_DEBUG, "Plugin Mapping Request : %s\n", request.c_str());
    return request;
}

bool UDADataSource::copy_from_cache(ram_cache::RamCache* ram_cache, DATA_BLOCK* data_block, const json_mapping::MapArguments& arguments, const std::string& request_str) const
{
    if (!m_cache_enabled) {
        return false;
    }

    auto signal_type = arguments.sig_type;

    using json_mapping::SignalType;
    switch (signal_type) {
        case SignalType::DATA:
            return ram_cache->copy_data_from_cache(request_str, data_block);
        case SignalType::ERROR:
            return ram_cache->copy_error_high_from_cache(request_str, data_block);
        case SignalType::TIME:
            return ram_cache->copy_time_from_cache(request_str, data_block);
        case SignalType::DIM:
            return ram_cache->copy_dim_from_cache(request_str, 1, data_block);
        default:
            return ram_cache->copy_from_cache(request_str, data_block);
    }
}

int UDADataSource::call_plugins(DATA_BLOCK* data_block, const json_mapping::DataSourceArgs& data_source_args, const json_mapping::MapArguments& arguments, ram_cache::RamCache* ram_cache, std::optional<float> scale, std::optional<float> offset, std::optional<std::string> slice) const
{
    int err{1};
    auto request_str = get_request_str(data_source_args, arguments, std::move(slice));
    if (request_str.empty()) {
        return err;
    } // Return 1 if no request receieved

    /*
     *
     * generate subset info then remove subset syntax from
     *  request string
     *
     */

    REQUEST_DATA request = {0};
    strcpy(request.signal, request_str.c_str());

    ENVIRONMENT* environment = getIdamClientEnvironment();
    makeRequestData(&request, *m_plugin_list, environment);

    SUBSET data_subset = request.datasubset;
    json_mapping::subset::log_request_status(&request, "request block before interception: ");

    // assume subsetting is requested if the final part of the request string is
    // in sqaure bracktes
    if (request_str.back() == ']' && request_str.rfind('[') != std::string::npos) {
        std::size_t subset_syntax_position = request_str.rfind('[');
        if (m_cache_enabled) {
            ram_cache->log(ram_cache::LogLevel::INFO, "request before alteration: " + request_str);
        }
        request_str.erase(subset_syntax_position);
        if (m_cache_enabled) {
            ram_cache->log(ram_cache::LogLevel::INFO, "request after alteration: " + request_str);
        }
    }

    if (m_cache_enabled) {
        std::string key_found = ram_cache->has_entry(request_str) ? "True" : "False";
        ram_cache->log(ram_cache::LogLevel::DEBUG, "key, \"" + request_str + "\" in cache? " + key_found);
    }

    /*
     *
     * CACHING GOES HERE
     *
     */

    // check cache for request string and only get data if it's not already there
    // currently copies whole datablock (data, error, and dims)
    if (m_cache_enabled) {
        ram_cache->log(ram_cache::LogLevel::DEBUG, "caching disbaled");
    }

    bool cache_hit = copy_from_cache(ram_cache, data_block, arguments, request_str);
    if (cache_hit) {
        ram_cache->log(ram_cache::LogLevel::INFO, "Adding cached datablock onto plugin_interface");
        ram_cache->log(ram_cache::LogLevel::INFO,
                         "data on plugin_interface (data_n): " + std::to_string(data_block->data_n));
        err = 0;
    } else {
        IDAM_PLUGIN_INTERFACE interface = {0};
        CLIENT_BLOCK client_block;
        DATA_SOURCE data_source;
        SIGNAL_DESC signal_desc;
        initClientBlock(&client_block, 0, "");
        initDataSource(&data_source);
        initSignalDesc(&signal_desc);

        interface.request_data = &request;
        interface.pluginList = m_plugin_list;
        interface.data_block = data_block;
        interface.environment = environment;
        interface.client_block = &client_block;
        interface.data_source = &data_source;
        interface.signal_desc = &signal_desc;

        err = callPlugin(m_plugin_list, request_str.c_str(), &interface);
        json_mapping::subset::log_request_status(&request, "request block status:");

        if (err != 0) {
            // add check of int udaNumErrors() and if more than one, don't wipe
            // 220 situation when UDA tries to get data and cannot find it
            if (err == 220) {
                closeUdaError();
            }
            return err;
        } // return code if failure, no need to proceed

        // Add retrieved datablock to cache. data is copied from datablock into a new ram_cache::data_entry. original
        // data remains on block (on plugin_interface structure) for return.
        if (m_cache_enabled) {
            ram_cache->add(request_str, data_block);
        }
    }

    const char* subset_method = getenv("UDA_JSON_MAPPING_SUBSET_METHOD");

    using json_mapping::SignalType;

    // set serverside subsetting as default unless new method is specifically requested.
    bool use_plugin_subset = (subset_method != nullptr) && (StringIEquals(subset_method, "PLUGIN_SUBSET"));
    // TODO: handle dim data scaling (hardcoded to disable scaling here)
    bool dim_data = arguments.sig_type == SignalType::DIM || arguments.sig_type == SignalType::TIME;

    if (data_subset.nbound > 0) {
        auto scale_value = (!dim_data && scale.has_value()) ? scale.value() : 1.0;
        json_mapping::subset::log(json_mapping::subset::LogLevel::INFO, "scale factor is: " + std::to_string(scale_value));
        auto offset_value = (!dim_data && offset.has_value()) ? offset.value() : 0.0;
        json_mapping::subset::log(json_mapping::subset::LogLevel::INFO, "offset factor is: " + std::to_string(offset_value));
        json_mapping::subset::apply_subsetting(data_block, data_subset, scale_value, offset_value);
    }

    return err;
}

json_mapping::TypedDataArray UDADataSource::get(const json_mapping::DataSourceArgs& data_source_args, const json_mapping::MapArguments& arguments, ram_cache::RamCache* ram_cache, std::optional<float> scale, std::optional<float> offset, std::optional<std::string> slice)
{
    DATA_BLOCK data_block;
    int err = call_plugins(&data_block, data_source_args, arguments, ram_cache, scale, offset, slice);

    // temporary solution to the slice functionality returning arrays of 1 element
    if (data_block.rank == 1 && data_block.data_n == 1) {
        data_block.rank = 0;
    }

    size_t size = data_block.data_n;
    std::vector<size_t> shape(data_block.rank);
    for (int i = 0; i < data_block.rank; ++i) {
        shape[i] = data_block.dims[i].dim_n;
    }

    switch (data_block.data_type) {
        case UDA_TYPE_INT:
            return json_mapping::TypedDataArray{ reinterpret_cast<int*>(data_block.data), size, shape };
        case UDA_TYPE_FLOAT:
            return json_mapping::TypedDataArray{ reinterpret_cast<float*>(data_block.data), size, shape };
        case UDA_TYPE_DOUBLE:
            return json_mapping::TypedDataArray{ reinterpret_cast<double*>(data_block.data), size, shape };
        default:
            throw std::runtime_error{ "unknown data type" };
    }
}
