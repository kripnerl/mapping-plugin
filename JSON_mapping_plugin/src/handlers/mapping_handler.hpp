#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <filesystem>

#include "map_types/base_mapping.hpp"
#include "utils/ram_cache.hpp"
#include <nlohmann/json.hpp>

using IDSName = std::string;
using MachineName = std::string;
using MappingName = std::string;

template <typename T>
struct MapSelector {
    std::unordered_map<int, T> map;
};

template <typename T>
int select_shot(const MapSelector<T>& selector, int shot) {
    int selected = -1;
    for (auto el : selector.map) {
        int key = el.first;
        if (shot > selected) {
            selected = key;
        }
    }
    return selected;
}

using IDSMapRegister = std::unordered_map<MappingName, std::unique_ptr<Mapping>>;
using IDSMapRegisterStore = std::unordered_map<IDSName, MapSelector<IDSMapRegister>>;

using IDSGlobals = nlohmann::json;
using IDSGlobalsStore = std::unordered_map<IDSName, MapSelector<IDSGlobals>>;

struct MachineMapping {
    IDSMapRegisterStore mappings;
    IDSGlobalsStore attributes;
};

using MachineRegisterStore = std::unordered_map<MachineName, MachineMapping>;
using MappingPair = std::pair<nlohmann::json&, IDSMapRegister&>;

class MappingHandler
{

  public:
    MappingHandler() : m_init(false), m_dd_version("3.39.0"), m_cache_enabled(false) {};
    explicit MappingHandler(std::string dd_version) : m_init(false), m_dd_version(std::move(dd_version)), m_cache_enabled(false) {};

    int reset();
    int init();
    int set_map_dir(const std::string& mapping_dir);
    std::optional<MappingPair> read_mappings(const MachineName& machine, const std::string& request_ids, const REQUEST_DATA* request_data);

  private:
    [[nodiscard]] std::vector<int> find_mapping_dirs(const MachineName& machine, const IDSName& ids_name) const;
    [[nodiscard]] std::filesystem::path mapping_path(const MachineName& machine, const IDSName& ids_name, int shot, const std::string& file_name) const;
    int load_machine(const MachineName& machine);
    [[nodiscard]] nlohmann::json load_toplevel(const MachineName& machine) const;
    int load_shot_globals(const MachineName& machine, const IDSName& ids_name, int shot);
    int load_globals(const MachineName& machine, const IDSName& ids_name);
    int load_shot_mappings(const MachineName& machine, const IDSName& ids_name, int shot);
    int load_mappings(const MachineName& machine, const IDSName& ids_name);

    int init_mappings(const MachineName& machine, const IDSName& ids_name, const nlohmann::json& data, int shot);
    static int init_value_mapping(IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value);
    static int init_plugin_mapping(IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value,
                                   const nlohmann::json& ids_attributes,
                                   std::shared_ptr<ram_cache::RamCache>& ram_cache);
    static int init_dim_mapping(IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value);
    // static int init_slice_mapping(IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value);
    static int init_expr_mapping(IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value);
    static int init_custom_mapping(IDSMapRegister& map_reg, const std::string& key, const nlohmann::json& value);

    MachineRegisterStore m_machine_register;
    bool m_init;

    std::string m_dd_version;
    std::filesystem::path m_mapping_dir;
    nlohmann::json m_mapping_config;
    std::shared_ptr<ram_cache::RamCache> m_ram_cache;
    bool m_cache_enabled;
};
