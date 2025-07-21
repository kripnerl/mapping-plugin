#include "mapping_plugin.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>

// LibTokaMap includes
#include <handlers/mapping_handler.hpp>
#include <map_types/data_source_mapping.hpp>

// UDA includes
#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <logging/logging.h>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <server/getServerEnvironment.h>

#include "uda_data_source.hpp"
#include "uda_plugin_helpers.hpp"

namespace
{

enum class LogLevel : uint8_t { DEBUG, INFO, WARNING, ERROR };

/**
 * @brief Temporary logging function for libtokamap_plugin, outputs
 * to UDA_HOME/etc/
 *
 * @param log_level The LogLevel (INFO, WARNING, ERROR, DEBUG)
 * @param log_msg The message to be logged
 * @return
 */
int log(LogLevel log_level, std::string_view log_msg)
{
    const ENVIRONMENT* environment = getServerEnvironment();

    std::string const log_file_name = std::string{static_cast<const char*>(environment->logdir)} + "/JSON_plugin.log";
    std::ofstream log_file;
    log_file.open(log_file_name, std::ios_base::out | std::ios_base::app);
    std::time_t const time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto timestamp = std::put_time(std::gmtime(&time_now), "%Y-%m-%d:%H:%M:%S"); // NOLINT(concurrency-mt-unsafe)
    if (!log_file) {
        return 1;
    }

    switch (log_level) {
        case LogLevel::DEBUG:
            log_file << timestamp << ":DEBUG - ";
            break;
        case LogLevel::INFO:
            log_file << timestamp << ":INFO - ";
            break;
        case LogLevel::WARNING:
            log_file << timestamp << ":WARNING - ";
            break;
        case LogLevel::ERROR:
            log_file << timestamp << ":ERROR - ";
            break;
        default:
            log_file << "LOG_LEVEL NOT DEFINED";
    }
    log_file << log_msg << "\n";
    log_file.close();

    return 0;
}

/**
 * @class JSONMappingPlugin
 * @brief UDA plugin to map Tokamak data
 *
 * UDA plugin to allow the mapping of experimental fusion data to IMAS data
 * format for a given data dictionary version. Mappings are available in
 * JSON format, which are then parsed and used to
 * (1) read data,
 * (2) apply transformations, and
 * (3) return the data in a format the IDS is expecting
 *
 */
class JSONMappingPlugin
{
  public:
    int entry_handle(IDAM_PLUGIN_INTERFACE* plugin_interface);

  private:
    int execute(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int init(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int reset(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int get(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int help(IDAM_PLUGIN_INTERFACE* plugin_interface);
    static int version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    static int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);
    static int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);
    static int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);

    // Loads, controls, stores mapping file lifetime
    libtokamap::MappingHandler m_mapping_handler;
    bool m_init = false;
    std::string m_request_function;
};

/**
 * @brief Initialise the libtokamap_plugin
 *
 * Set mapping directory and load mapping files into mapping_handler
 * RAISE_PLUGIN_ERROR if JSON mapping file location is not set
 *
 * @param plugin_interface Top-level UDA plugin interface
 * @return errorcode UDA convention to return int errorcode
 * 0 success, !0 failure
 */
int JSONMappingPlugin::init(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    if (!m_init || m_request_function == "init" || m_request_function == "initialise") {
        reset(plugin_interface);
    }

    const char* map_dir = getenv("UDA_MAPPING_DIR"); // NOLINT(concurrency-mt-unsafe)
    if (map_dir == nullptr) {
        log(LogLevel::ERROR, "JSONMappingPlugin::init: - JSON mapping locations not set");
        RAISE_PLUGIN_ERROR("JSONMappingPlugin::init: - JSON mapping locations not set")
    }

    nlohmann::json config = {{"mapping_directory", std::string{map_dir}}};

    const char* use_cache = getenv("UDA_MAPPING_USE_CACHE");
    if (use_cache != nullptr) {
        config["use_cache"] = bool(std::stoi(std::string{use_cache}));
    }

    const char* cache_size = getenv("UDA_MAPPING_USE_CACHE");
    if (use_cache != nullptr) {
        config["cache_size"] = std::stoi(std::string{cache_size});
    }

    m_mapping_handler.init(config);

    auto data_source = std::make_unique<json_plugin::UDADataSource>("UDA", "get", plugin_interface->pluginList, false);
    libtokamap::DataSourceMapping::register_data_source("UDA", std::move(data_source));

    m_init = true;

    return 0;
}

/**
 * @brief
 *
 * @param plugin_interface Top-level UDA plugin interface
 * @return errorcode UDA convention to return int errorcode
 * 0 success, !0 failure
 */
int JSONMappingPlugin::reset(IDAM_PLUGIN_INTERFACE* /*plugin_interface*/) // silence unused warning
{
    if (m_init) {
        // Free Heap & reset counters if initialised
        libtokamap::DataSourceMapping::unregister_data_source("UDA");
        m_init = false;
    }
    return 0;
}

void add_machine_specific_attributes(IDAM_PLUGIN_INTERFACE* plugin_interface, nlohmann::json& attributes)
{
    for (int i = 0; i < plugin_interface->request_data->nameValueList.pairCount; ++i) {
        std::string const name = plugin_interface->request_data->nameValueList.nameValue[i].name;
        std::string const value = plugin_interface->request_data->nameValueList.nameValue[i].value;

        char* p_end = nullptr;
        long const i_value = std::strtol(value.c_str(), &p_end, 10);
        if (*p_end == '\0') {
            attributes[name] = i_value;
        } else {
            attributes[name] = value;
        }
    }
}

/**
 * @brief Main data/mapping function called from class entry function
 *
 * Arguments:
 *  - machine   string      the name of the machine to map data for
 *  - path      string      the IDS path we need to map data for
 *  - rank      int         the rank of the data expected
 *  - shape     int array   the shape of the data expected
 *  - datatype  UDA_TYPE    the type of the data expected
 *  - <machine specific args> any remaining arguments are specific to the machine and have been passed via query
 *    arguments on the URI, i.e. imas://server/uda?machine=MASTU&shot=30420&run=1 would pass shot and run as additional
 *    arguments
 *
 * @param plugin_interface Top-level UDA plugin interface
 * @return UDA convention to return int error code (0 success, !0 failure)
 */
int JSONMappingPlugin::get(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    REQUEST_DATA* request_data = plugin_interface->request_data;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = nullptr;

    const char* mapping = nullptr;
    const char* path = nullptr;
    int datatype = UDA_TYPE_UNKNOWN;
    int rank = -1;

    FIND_REQUIRED_STRING_VALUE(request_data->nameValueList, mapping)
    FIND_REQUIRED_STRING_VALUE(request_data->nameValueList, path)
    FIND_REQUIRED_INT_VALUE(request_data->nameValueList, datatype);
    if (datatype < 0 || datatype > UDA_TYPE_CAPNP) {
        throw std::runtime_error{"Invalid datatype"};
    }
    FIND_REQUIRED_INT_VALUE(request_data->nameValueList, rank);
    if (rank < 0) {
        throw std::runtime_error{"Invalid rank"};
    }

    nlohmann::json extra_attributes = {};
    add_machine_specific_attributes(plugin_interface, extra_attributes);

    auto type_index = std::type_index{typeid(void)};
    switch (datatype) {
        case UDA_TYPE_INT:
            type_index = std::type_index{typeid(int)};
            break;
        case UDA_TYPE_FLOAT:
            type_index = std::type_index{typeid(float)};
            break;
        case UDA_TYPE_DOUBLE:
            type_index = std::type_index{typeid(double)};
            break;
        case UDA_TYPE_STRING:
            type_index = std::type_index{typeid(const char)};
            break;
        default:
            break;
    }

    auto array = m_mapping_handler.map(mapping, path, type_index, rank, extra_attributes);
    json_plugin::set_data_block(data_block, array);

    return 0;
}

int JSONMappingPlugin::execute(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    int return_code = 0;
    if (m_request_function == "help") {
        return_code = JSONMappingPlugin::help(plugin_interface);
    } else if (m_request_function == "version") {
        return_code = JSONMappingPlugin::version(plugin_interface);
    } else if (m_request_function == "builddate") {
        return_code = JSONMappingPlugin::build_date(plugin_interface);
    } else if (m_request_function == "defaultmethod") {
        return_code = JSONMappingPlugin::default_method(plugin_interface);
    } else if (m_request_function == "maxinterfaceversion") {
        return_code = JSONMappingPlugin::max_interface_version(plugin_interface);
    } else if (m_request_function == "read" || m_request_function == "get") {
        return_code = get(plugin_interface);
    } else if (m_request_function == "close") {
        return_code = 0;
    } else {
        throw std::runtime_error{"Unknown function requested"};
    }
    return return_code;
}

/**
 * entry_handle: plugin C++ class entry point from C access
 * and set current function call to plugin
 * @param plugin_interface
 * @return
 */
int JSONMappingPlugin::entry_handle(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    // set current function
    m_request_function = static_cast<const char*>(plugin_interface->request_data->function);

    // housekeeping
    if (plugin_interface->housekeeping != 0 || m_request_function == "reset") {
        reset(plugin_interface);
        return 0;
    }

    // Initialise
    init(plugin_interface);
    if (m_request_function == "init" || m_request_function == "initialise") {
        return 0;
    }

    return execute(plugin_interface);
}

/**
 * Help: A Description of library functionality
 * @param plugin_interface
 * @return
 */
int JSONMappingPlugin::help(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* help = "\nJSONMappingPlugin: Add Functions Names, Syntax, and "
                       "Descriptions\n\n";
    const char* desc = "templatePlugin: help = description of this plugin";

    return setReturnDataString(plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param plugin_interface
 * @return
 */
int JSONMappingPlugin::version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param plugin_interface
 * @return
 */
int JSONMappingPlugin::build_date(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param plugin_interface
 * @return
 */
int JSONMappingPlugin::default_method(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param plugin_interface
 * @return
 */
int JSONMappingPlugin::max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION,
                                  "Maximum Interface Version");
}

} // namespace

/**
 * @brief Plugin entry function
 *
 * @param plugin_interface
 * @return
 */
[[maybe_unused]] int jsonMappingPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!")
    }

    plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    try {
        static JSONMappingPlugin plugin = {};
        return plugin.entry_handle(plugin_interface);
    } catch (const std::exception& ex) {
        RAISE_PLUGIN_ERROR_EX(ex.what(), { concatUdaError(&plugin_interface->error_stack); })
    }
}
