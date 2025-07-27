#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

#include "map_types/map_arguments.hpp"
#include "utils/mapping_locator.hpp"
#include "utils/ram_cache.hpp"
#include "utils/types.hpp"
#include "valijson_nlohmann_bundled.hpp"

struct PluginList;

namespace libtokamap
{

class MappingHandler
{
  public:
    MappingHandler() : m_init(false), m_dd_version("3.42.0"), m_cache_enabled(false) {};
    explicit MappingHandler(std::string dd_version)
        : m_init(false), m_dd_version(std::move(dd_version)), m_cache_enabled(false) {};

    void reset();
    void init(const nlohmann::json& config);

    [[nodiscard]] TypedDataArray map(const std::string& experiment, const std::string& path, std::type_index data_type,
                                     int rank, const nlohmann::json& extra_attributes);

  private:
    void load_experiment(const ExperimentName& experiment, const nlohmann::json& attributes);

    ExperimentRegisterStore m_experiment_register;
    bool m_init;

    std::string m_dd_version;
    std::filesystem::path m_mapping_dir;
    std::shared_ptr<libtokamap::RamCache> m_ram_cache;
    bool m_cache_enabled;
    valijson::Schema m_mappings_schema;
    valijson::Schema m_globals_schema;
    valijson::Schema m_mapping_config_schema;
};

} // namespace libtokamap
