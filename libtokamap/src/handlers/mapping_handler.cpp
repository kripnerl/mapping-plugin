#include "mapping_handler.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <cstddef>
#include <cstdint>
#include <cxxabi.h>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <inja/inja.hpp>
#include <iomanip>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gsl/gsl-lite.hpp"
#include "map_types/base_mapping.hpp"
#include "map_types/custom_mapping.hpp"
#include "map_types/data_source_mapping.hpp"
#include "map_types/dim_mapping.hpp"
#include "map_types/expr_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "map_types/value_mapping.hpp"
#include "utils/indices.hpp"
#include "utils/ram_cache.hpp"
#include "utils/syntax_parser.hpp"

namespace
{
std::string to_string(const std::type_index& value)
{
    int status = 0;
    return abi::__cxa_demangle(value.name(), nullptr, nullptr, &status);
}

std::ostream& operator<<(std::ostream& out, const std::type_index& value)
{
    int status = 0;
    out << abi::__cxa_demangle(value.name(), nullptr, nullptr, &status);
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::vector<size_t>& shape)
{
    out << "[";
    const char* delim = "";
    for (const auto element : shape) {
        out << delim << element;
        delim = ",";
    }
    out << "]";
    return out;
}

template <typename T>
    requires(std::is_arithmetic_v<T>)
std::ostream& operator<<(std::ostream& out, const gsl::span<const T>& data)
{
    out << "[";
    out << std::setprecision(3);
    const char* delim = "";
    constexpr size_t max = 5;
    size_t count = 0;
    for (const auto element : data) {
        if (count == max) {
            out << ",...";
            break;
        }
        out << delim << element;
        delim = ",";
        ++count;
    }
    out << "]";
    return out;
}

template <typename T> void print(std::ostream& out, const char* buffer, size_t size)
{
    gsl::span<const T> data{reinterpret_cast<const T*>(buffer), size};
    out << ", data=" << data;
}
} // namespace

std::string libtokamap::TypedDataArray::to_string() const
{
    std::stringstream out;
    out << "{ type=" << m_type_index << ", size=" << m_size << ", shape=" << m_shape;
    switch (type_index_map(m_type_index)) {
        case DataType::Char:
            print<char>(out, m_buffer, m_size);
            break;
        case DataType::Short:
            print<short>(out, m_buffer, m_size);
            break;
        case DataType::Int:
            print<int>(out, m_buffer, m_size);
            break;
        case DataType::Long:
            print<long>(out, m_buffer, m_size);
            break;
        case DataType::Int64:
            print<int64_t>(out, m_buffer, m_size);
            break;
        case DataType::UChar:
            print<unsigned char>(out, m_buffer, m_size);
            break;
        case DataType::UShort:
            print<unsigned short>(out, m_buffer, m_size);
            break;
        case DataType::UInt:
            print<unsigned int>(out, m_buffer, m_size);
            break;
        case DataType::ULong:
            print<unsigned long>(out, m_buffer, m_size);
            break;
        case DataType::UInt64:
            print<uint64_t>(out, m_buffer, m_size);
            break;
        case DataType::Float:
            print<float>(out, m_buffer, m_size);
            break;
        case DataType::Double:
            print<double>(out, m_buffer, m_size);
            break;
        default:
            throw std::runtime_error{"unhandled data type: '" + ::to_string(m_type_index) + "'"};
    }
    out << " }";
    return out.str();
}

void libtokamap::MappingHandler::reset()
{
    m_machine_register.clear();
    m_mapping_config.clear();
    m_init = false;
}

void libtokamap::MappingHandler::init(const nlohmann::json& config)
{
    if (m_init || !m_machine_register.empty()) {
        return;
    }

    if (!config.contains("mapping_directory")) {
        throw std::runtime_error{"mapping_directory not specified in config"};
    }
    m_mapping_dir = config.at("mapping_directory").get<std::string>();

    bool enable_caching = config.contains("use_cache") && config.at("use_cache").get<bool>();

    if (enable_caching) {
        const std::size_t cache_size =
            config.contains("cache_size") ? config.at("cache_size").get<int>() : libtokamap::default_size;
        m_ram_cache = std::make_shared<libtokamap::RamCache>(cache_size);
    } else {
        m_ram_cache = nullptr;
    }

    m_cache_enabled = m_ram_cache != nullptr;
    m_init = true;
}

libtokamap::TypedDataArray libtokamap::MappingHandler::map(const std::string& mapping, const std::string& path,
                                                           std::type_index data_type, int rank,
                                                           const nlohmann::json& extra_attributes)
{
    std::deque<std::string> path_tokens;
    boost::split(path_tokens, path, boost::is_any_of("/"));
    if (path_tokens.empty()) {
        throw std::runtime_error{"IDS path could not be split"};
    }

    auto [indices, new_tokens] = extract_indices(path_tokens);

    // Use first hash of the IDS path as the IDS name
    std::string const ids_name{new_tokens.front()};

    // Use lowercase machine name for find mapping files
    std::string machine_string = mapping;
    boost::to_lower(machine_string);

    // Load mappings based off IDS name
    // Returns a reference to IDS map objects and corresponding globals
    // Mapping object lifetime owned by mapping_handler
    const auto maybe_mappings = read_mappings(machine_string, ids_name, extra_attributes);

    if (!maybe_mappings) {
        throw std::runtime_error{"no mappings found for machine '" + machine_string + "' and IDS '" + ids_name + "'"};
    }

    const auto& [attributes, mappings] = maybe_mappings.value();

    // Remove IDS name from path and rejoin for hash map key
    // magnetics/coil/#/current -> coil/#/current
    new_tokens.pop_front();

    const auto sig_type = deduce_signal_type(new_tokens.back());
    std::string const map_path = generate_map_path(new_tokens, indices, mappings, path);
    if (map_path.empty()) {
        throw std::runtime_error{"failed to find mapping for '" + path + "'"};
    }

    // Add request indices to globals
    attributes["indices"] = indices;

    for (const auto& [key, value] : extra_attributes.items()) {
        attributes[key] = value;
    }

    const libtokamap::MapArguments map_arguments{mappings, attributes, sig_type, data_type, rank};

    return mappings.at(map_path)->map(map_arguments);
}

std::optional<libtokamap::MappingPair> libtokamap::MappingHandler::read_mappings(const MachineName& machine,
                                                                                 const std::string& request_ids,
                                                                                 const nlohmann::json& extra_attributes)
{
    int shot = 0;
    const bool shot_found = extra_attributes.contains("shot");
    if (shot_found) {
        shot = extra_attributes["shot"];
    }

    load_machine(machine);
    if (!m_machine_register.contains(machine)) {
        return {};
    }

    auto& [mappings, attributes] = m_machine_register[machine];
    if (!mappings.contains(request_ids) || !attributes.contains(request_ids)) {
        return {};
    }

    int attr_shot = -1;
    int map_shot = -1;

    if (shot_found) {
        attr_shot = select_shot(attributes[request_ids], shot);
        map_shot = select_shot(attributes[request_ids], shot);
    }

    if (!attributes[request_ids].map.contains(attr_shot) || !mappings[request_ids].map.contains(attr_shot)) {
        return {};
    }

    nlohmann::json& attr = attributes[request_ids].map[attr_shot];
    IDSMapRegister& map = mappings[request_ids].map[map_shot];

    // AJP :: Safety check if ids request not in mapping json (and typo obviously)
    return {std::make_pair(std::ref(attr), std::ref(map))};
}

std::vector<int> libtokamap::MappingHandler::find_mapping_dirs(const MachineName& machine,
                                                               const IDSName& ids_name) const
{
    auto path = m_mapping_dir / machine / ids_name;
    std::vector<int> mapping_dirs;

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            mapping_dirs.push_back(std::stoi(entry.path().filename().string()));
        }
    }

    return mapping_dirs;
}

std::filesystem::path libtokamap::MappingHandler::mapping_path(const MachineName& machine, const IDSName& ids_name,
                                                               const int shot, const std::string& file_name) const
{
    if (ids_name.empty()) {
        return m_mapping_dir / machine / file_name;
    }

    if (shot < 0) {
        return m_mapping_dir / machine / ids_name / file_name;
    }

    return m_mapping_dir / machine / ids_name / std::to_string(shot) / file_name;
}

void libtokamap::MappingHandler::load_machine(const MachineName& machine)
{
    if (m_machine_register.count(machine) == 1) {
        // machine already loaded
        return;
    }

    const auto file_path = mapping_path(machine, "", 0, "mappings.cfg.json");

    std::ifstream map_cfg_file(file_path);
    if (map_cfg_file) {
        map_cfg_file >> m_mapping_config;
    } else {
        throw std::runtime_error{"Cannot open JSON mapping config file"};
    }

    m_machine_register[machine] = {.mappings = {}, .attributes = {}};

    for (const auto& ids_name : m_mapping_config[m_dd_version].get<std::vector<std::string>>()) {
        load_globals(machine, ids_name);
        load_mappings(machine, ids_name);
    }
}

nlohmann::json libtokamap::MappingHandler::load_toplevel(const MachineName& machine) const
{
    auto file_path = mapping_path(machine, "", 0, "globals.json");

    nlohmann::json toplevel_globals;

    std::ifstream globals_file;
    globals_file.open(file_path);
    if (globals_file) {
        try {
            globals_file >> toplevel_globals;
        } catch (nlohmann::json::exception& ex) {
            throw std::runtime_error{ex.what()};
        }
    } else {
        throw std::runtime_error{"Cannot open top-level globals file"};
    }
    return toplevel_globals;
}

void libtokamap::MappingHandler::load_shot_globals(const MachineName& machine, const IDSName& ids_name, int shot)
{
    auto file_path = mapping_path(machine, ids_name, shot, "globals.json");

    std::ifstream globals_file;
    globals_file.open(file_path);
    if (globals_file) {
        nlohmann::json temp_globals;
        try {
            globals_file >> temp_globals;
        } catch (nlohmann::json::exception& ex) {
            throw std::runtime_error{ex.what()};
        }

        temp_globals.update(load_toplevel(machine));
        m_machine_register[machine].attributes[ids_name].map[shot] = temp_globals; // Record globals
    } else {
        throw std::runtime_error{"Cannot open JSON globals file"};
    }
}

void libtokamap::MappingHandler::load_globals(const MachineName& machine, const IDSName& ids_name)
{
    const auto mapping_dirs = find_mapping_dirs(machine, ids_name);
    if (mapping_dirs.empty()) {
        load_shot_globals(machine, ids_name, -1);
    }
    for (const int shot : mapping_dirs) {
        load_shot_globals(machine, ids_name, shot);
    }
}

void libtokamap::MappingHandler::load_shot_mappings(const MachineName& machine, const IDSName& ids_name, int shot)
{
    auto file_path = mapping_path(machine, ids_name, shot, "mappings.json");

    std::ifstream map_file;
    map_file.open(file_path);
    if (map_file) {
        nlohmann::json temp_mappings;
        try {
            map_file >> temp_mappings;
        } catch (nlohmann::json::exception& ex) {
            throw std::runtime_error{ex.what()};
        }

        init_mappings(machine, ids_name, temp_mappings, shot);
    } else {
        throw std::runtime_error{"Cannot open JSON mapping file"};
    }
}

void libtokamap::MappingHandler::load_mappings(const MachineName& machine, const IDSName& ids_name)
{
    const auto mapping_dirs = find_mapping_dirs(machine, ids_name);
    if (mapping_dirs.empty()) {
        load_shot_mappings(machine, ids_name, -1);
    }
    for (const int shot : mapping_dirs) {
        load_shot_mappings(machine, ids_name, shot);
    }
}

namespace
{

void apply_config(std::unordered_map<std::string, nlohmann::json>& args, nlohmann::json plugin_config_map,
                  const std::string& plugin_name)
{
    if (plugin_config_map.contains(plugin_name)) {
        const auto& plugin_config = plugin_config_map[plugin_name].get<nlohmann::json>();
        const auto& plugin_args = plugin_config["ARGS"].get<nlohmann::json>();
        for (const auto& [name, arg] : plugin_args.items()) {
            if (!args.contains(name)) {
                // don't overwrite mapping arguments with global values
                args[name] = arg;
            }
        }
    }
}

std::optional<float> get_float_value(const std::string& name, const nlohmann::json& value,
                                     const nlohmann::json& ids_attributes)
{
    std::optional<float> opt_float{std::nullopt};
    if (value.contains(name) and !value[name].is_null()) {
        if (value[name].is_number()) {
            opt_float = value[name].get<float>();
        } else if (value[name].is_string()) {
            try {
                const auto post_inja_str = inja::render(value[name].get<std::string>(), ids_attributes);
                opt_float = std::stof(post_inja_str);
            } catch (const std::invalid_argument&) {
                // const std::string message = "\nCannot convert " + name + " string to float\n";
                // UDA_LOG(UDA_LOG_DEBUG, "%s", message.c_str());
            }
        }
    }
    return opt_float;
}

std::string find_mapping(libtokamap::IDSMapRegister& mappings, const std::string& path, const std::vector<int>& indices,
                         const std::string& full_path)
{
    // If mapping is found we are good
    if (mappings.contains(path)) {
        return path;
    }

    // Check with the path without generalisation
    if (mappings.contains(full_path)) {
        return full_path;
    }

    // If there's nothing to replace then no mapping can be found
    if (indices.empty()) {
        return "";
    }

    // Check for last # replaced with index
    std::string new_path = boost::replace_last_copy(path, "#", std::to_string(indices.back()));
    if (mappings.contains(new_path)) {
        return new_path;
    }

    // No mappings found
    return "";
}

void init_value_mapping(libtokamap::IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value)
{
    const auto& value_json = value.at("VALUE");
    map_reg.try_emplace(key, std::make_unique<libtokamap::ValueMapping>(value_json));
}

void init_data_source_mapping(libtokamap::IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value,
                              const nlohmann::json& ids_attributes, std::shared_ptr<libtokamap::RamCache>& ram_cache)
{
    if (!value.contains("DATA_SOURCE")) {
        throw std::runtime_error{"required DATA_SOURCE argument not provided in DATA_SOURCE mapping '" + key + "'"};
    }
    auto data_source_name = value["DATA_SOURCE"].get<std::string>();
    boost::to_upper(data_source_name);

    if (!value.contains("ARGS")) {
        throw std::runtime_error{"required ARGS argument not provided in DATA_SOURCE mapping '" + key + "'"};
    }
    auto args = value["ARGS"].get<libtokamap::DataSourceArgs>();
    auto offset = get_float_value("OFFSET", value, ids_attributes);
    auto scale = get_float_value("SCALE", value, ids_attributes);
    auto slice = value.contains("SLICE") ? std::optional<std::string>{value.at("SLICE").get<std::string>()}
                                         : std::optional<std::string>{};

    if (ids_attributes.contains("DATA_SOURCE_CONFIG")) {
        const auto& plugin_config_map = ids_attributes.at("DATA_SOURCE_CONFIG");
        apply_config(args, plugin_config_map, data_source_name);
    }

    map_reg.try_emplace(
        key, std::make_unique<libtokamap::DataSourceMapping>(data_source_name, args, offset, scale, slice, ram_cache));
}

void init_dim_mapping(libtokamap::IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value)
{
    map_reg.try_emplace(key, std::make_unique<libtokamap::DimMapping>(value["DIM_PROBE"].get<std::string>()));
}

void init_expr_mapping(libtokamap::IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value)
{
    map_reg.try_emplace(key, std::make_unique<libtokamap::ExprMapping>(
                                 value["EXPR"].get<std::string>(),
                                 value["PARAMETERS"].get<std::unordered_map<std::string, std::string>>()));
}

void init_custom_mapping(libtokamap::IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value)
{
    map_reg.try_emplace(
        key, std::make_unique<libtokamap::CustomMapping>(value["CUSTOM_TYPE"].get<libtokamap::CustomMapType_t>()));
}

} // namespace

void libtokamap::MappingHandler::init_mappings(const MachineName& machine, const IDSName& ids_name,
                                               const nlohmann::json& data, int shot)
{
    const auto& attributes = m_machine_register[machine].attributes;
    IDSMapRegister temp_map_reg;
    for (const auto& [key, value] : data.items()) {
        // Parse syntactic sugar
        auto parsed_value = libtokamap::parse(value);

        if (!parsed_value.contains("MAP_TYPE")) {
            throw std::runtime_error{"required MAP_TYPE argument not found in mapping '" + key + "'"};
        }

        // TODO: make this case insensitive?
        switch (parsed_value["MAP_TYPE"].get<MappingType>()) {
            case MappingType::VALUE:
                init_value_mapping(temp_map_reg, key, parsed_value);
                break;
            case MappingType::DATA_SOURCE:
                init_data_source_mapping(temp_map_reg, key, parsed_value, attributes.at(ids_name).map.at(shot),
                                         m_ram_cache);
                break;
            case MappingType::DIM:
                init_dim_mapping(temp_map_reg, key, parsed_value);
                break;
            case MappingType::EXPR:
                init_expr_mapping(temp_map_reg, key, parsed_value);
                break;
            case MappingType::CUSTOM:
                init_custom_mapping(temp_map_reg, key, parsed_value);
                break;
            default:
                break;
        }
    }

    m_machine_register[machine].mappings[ids_name].map[shot] = std::move(temp_map_reg);
}

std::string libtokamap::generate_map_path(std::deque<std::string>& path_tokens, const std::vector<int>& indices,
                                          IDSMapRegister& mappings, const std::string& full_path)
{
    const auto sig_type = deduce_signal_type(path_tokens.back());
    if (sig_type == SignalType::INVALID) {
        return {}; // Don't throw, go gentle into that good night
    }

    std::string map_path = boost::algorithm::join(path_tokens, "/");
    std::string found_path;

    if (!mappings.contains(map_path)) {
        if (sig_type == SignalType::TIME or sig_type == SignalType::DATA) {
            path_tokens.pop_back();
            map_path = boost::algorithm::join(path_tokens, "/");
        }
        found_path = find_mapping(mappings, map_path, indices, full_path);
    } else {
        found_path = map_path;
    }

    return found_path;
}
