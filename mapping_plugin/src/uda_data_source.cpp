#include "uda_data_source.hpp"

#include <cstddef>
#include <cstring>
#include <exception>
#include <inja/inja.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// UDA includes
#include <client/getEnvironment.h>
#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/parseXML.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <structures/struct.h>

#include "map_types/data_source_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "uda_ram_cache.hpp"
#include "utils/ram_cache.hpp"

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
std::string json_plugin::UDADataSource::get_request_str(const libtokamap::DataSourceArgs& data_source_args,
                                                        const libtokamap::MapArguments& arguments) const
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

    auto request = string_stream.str();
    // UDA_LOG(UDA_LOG_DEBUG, "Plugin Mapping Request : %s\n", request.c_str());
    return request;
}

int json_plugin::UDADataSource::call_plugins(DATA_BLOCK* data_block, const libtokamap::DataSourceArgs& data_source_args,
                                             const libtokamap::MapArguments& arguments,
                                             libtokamap::RamCache* ram_cache) const
{
    int err{1};
    auto request_str = get_request_str(data_source_args, arguments);
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

    // if (m_cache_enabled) {
    //     std::string key_found = ram_cache->has_entry(request_str) ? "True" : "False";
    //     ram_cache->log(libtokamap:LogLevel::DEBUG, "key, \"" + request_str + "\" in cache? " + key_found);
    // }

    /*
     *
     * CACHING GOES HERE
     *
     */

    // check cache for request string and only get data if it's not already there
    // currently copies whole datablock (data, error, and dims)
    // if (m_cache_enabled) {
    //     ram_cache->log(libtokamap::LogLevel::DEBUG, "caching disabled");
    // }

    bool cache_hit = false;
    if (m_cache_enabled && ram_cache != nullptr) {
        cache_hit = json_plugin::copy_from_cache(*ram_cache, request_str, data_block);
    }
    if (cache_hit) {
        // ram_cache->log(libtokamap:LogLevel::INFO, "Adding cached datablock onto plugin_interface");
        // ram_cache->log(libtokamap:LogLevel::INFO,
        //                  "data on plugin_interface (data_n): " + std::to_string(data_block->data_n));
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

        if (err != 0) {
            // add check of int udaNumErrors() and if more than one, don't wipe
            // 220 situation when UDA tries to get data and cannot find it
            if (err == 220) {
                closeUdaError();
            }
            return err;
        } // return code if failure, no need to proceed

        // Add retrieved datablock to cache. data is copied from datablock into a new libtokamap:data_entry. original
        // data remains on block (on plugin_interface structure) for return.
        if (m_cache_enabled && ram_cache != nullptr) {
            json_plugin::copy_to_cache(*ram_cache, request_str, data_block);
        }
    }

    return err;
}

namespace
{
template <typename T>
libtokamap::TypedDataArray set_return_data(DataBlock& data_block, size_t size, std::vector<size_t>&& shape)
{
    auto array = libtokamap::TypedDataArray{reinterpret_cast<T*>(data_block.data), size, std::move(shape), false};
    // we set the data_block.data to nullptr to avoid double deletion
    data_block.data = nullptr;
    return array;
}
} // namespace

libtokamap::TypedDataArray json_plugin::UDADataSource::get(const libtokamap::DataSourceArgs& data_source_args,
                                                           const libtokamap::MapArguments& arguments,
                                                           libtokamap::RamCache* ram_cache)
{
    DATA_BLOCK data_block;
    int err = call_plugins(&data_block, data_source_args, arguments, ram_cache);

    if (err != 0) {
        return {};
    }

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
            return set_return_data<int>(data_block, size, std::move(shape));
        case UDA_TYPE_FLOAT:
            return set_return_data<float>(data_block, size, std::move(shape));
        case UDA_TYPE_DOUBLE:
            return set_return_data<double>(data_block, size, std::move(shape));
        case UDA_TYPE_STRING:
            return set_return_data<char>(data_block, size, std::move(shape));
        default:
            throw std::runtime_error{"unknown data type"};
    }
}
