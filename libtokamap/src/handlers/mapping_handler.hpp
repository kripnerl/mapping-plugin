#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

struct PluginList;

namespace libtokamap
{

using IDSName = std::string;
using MachineName = std::string;
using MappingName = std::string;

template <typename T> struct MapSelector {
    std::unordered_map<int, T> map;
};

template <typename T> int select_shot(const MapSelector<T>& selector, int shot)
{
    int selected = -1;
    for (auto element : selector.map) {
        int key = element.first;
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
    MappingHandler() : m_init(false), m_dd_version("3.42.0"), m_cache_enabled(false) {};
    explicit MappingHandler(std::string dd_version)
        : m_init(false), m_dd_version(std::move(dd_version)), m_cache_enabled(false) {};

    void reset();
    void init(const nlohmann::json& config);

    [[nodiscard]] TypedDataArray map(const std::string& mapping, const std::string& path, std::type_index data_type,
                                     int rank, const nlohmann::json& extra_attributes);

  private:
    [[nodiscard]] std::optional<MappingPair> read_mappings(const MachineName& machine, const std::string& request_ids,
                                                           const nlohmann::json& extra_attributes);
    [[nodiscard]] std::vector<int> find_mapping_dirs(const MachineName& machine, const IDSName& ids_name) const;
    [[nodiscard]] std::filesystem::path mapping_path(const MachineName& machine, const IDSName& ids_name, int shot,
                                                     const std::string& file_name) const;
    void load_machine(const MachineName& machine);
    [[nodiscard]] nlohmann::json load_toplevel(const MachineName& machine) const;
    void load_shot_globals(const MachineName& machine, const IDSName& ids_name, int shot);
    void load_globals(const MachineName& machine, const IDSName& ids_name);
    void load_shot_mappings(const MachineName& machine, const IDSName& ids_name, int shot);
    void load_mappings(const MachineName& machine, const IDSName& ids_name);

    void init_mappings(const MachineName& machine, const IDSName& ids_name, const nlohmann::json& data, int shot);

    MachineRegisterStore m_machine_register;
    bool m_init;

    std::string m_dd_version;
    std::filesystem::path m_mapping_dir;
    nlohmann::json m_mapping_config;
    std::shared_ptr<libtokamap::RamCache> m_ram_cache;
    bool m_cache_enabled;
};

std::string generate_map_path(std::deque<std::string>& path_tokens, const std::vector<int>& indices,
                              IDSMapRegister& mappings, const std::string& full_path);

} // namespace libtokamap
