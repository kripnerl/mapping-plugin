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
#include <clientserver/compressDim.h>

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
    // string_stream << m_plugin_name << "::" << m_function.value_or("get") << "(";
    string_stream << m_plugin_name << "::";

    if ( data_source_args.count("function") != 0 ) {
        string_stream << data_source_args.at("function").get<std::string>();
    } else {
        string_stream << m_function.value_or("get");
    }
    string_stream << "(";

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

    REQUEST_DATA request = {0};
    strcpy(request.signal, request_str.c_str());

    ENVIRONMENT* environment = getIdamClientEnvironment();
    makeRequestData(&request, *m_plugin_list, environment);

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

    // I think typically this datablock is just a shallow copy from the client-cached datablock list
    // so will be cleaned up anyway later
    // freeDataBlock(&data_block);
    return array;
}

template <typename T>
libtokamap::TypedDataArray set_return_dim(DataBlock& data_block, size_t index)
{
    auto dim = data_block.dims[index];
    size_t size = dim.dim_n;
    auto array = libtokamap::TypedDataArray{reinterpret_cast<T*>(dim.dim), size, {size}, false};
    // we set the data_block.data to nullptr to avoid double deletion
    dim.dim = nullptr;
    return array;
}

void expand_compressed_dim(DIMS& dim)
{
    if (dim.compressed == 0) 
    {
        return;
    }
    uncompressDim(&dim);
    dim.compressed = 0;
    dim.method = 0;
    if (dim.sams != nullptr) {
        free(dim.sams);
        dim.sams = nullptr;
    }
    if (dim.offs != nullptr) {
        free(dim.offs);
        dim.offs = nullptr;
    }
    if (dim.ints != nullptr) {
        free(dim.ints);
        dim.ints = nullptr;
    }
    dim.udoms = 0;
}

libtokamap::TypedDataArray set_return_time(DataBlock& data_block)
{
    size_t index = data_block.order;
    auto dim = data_block.dims[index];
    expand_compressed_dim(dim);
    switch (dim.data_type) {
        case UDA_TYPE_INT:
            return set_return_dim<int>(data_block, index);
        case UDA_TYPE_FLOAT:
            return set_return_dim<float>(data_block, index);
        case UDA_TYPE_DOUBLE:
            return set_return_dim<double>(data_block, index);
        case UDA_TYPE_STRING:
            return set_return_dim<char>(data_block, index);
        default:
            throw std::runtime_error{"unknown data type"};
    }
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

    if (data_source_args.count("time") != 0 && data_source_args.at("time").get<bool>()){
        return set_return_time(data_block);
    }

    size_t size = data_block.data_n;
    std::vector<size_t> shape{data_block.rank};
    for (int i = 0; i < data_block.rank; ++i) {
        shape[i] = data_block.dims[i].dim_n;
    }

    //note set_return_data destroys the data_block after moving the data out of it
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
