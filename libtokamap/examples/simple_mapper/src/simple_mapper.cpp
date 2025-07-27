#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <libtokamap.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <typeindex>
#include <utility>

#include "json_data_source.hpp"

namespace
{
unsigned long map(libtokamap::MappingHandler& mapping_handler, const std::string& mapping, const std::string& path)
{
    std::type_index data_type = std::type_index{typeid(char)};
    int rank = 1;
    nlohmann::json extra_attributes = {{"shot", 42}};

    auto result = mapping_handler.map(mapping, path, data_type, rank, extra_attributes);

    constexpr int64_t key_length = 35;
    std::string padding;
    padding.resize(std::max(key_length - static_cast<int64_t>(path.size()), int64_t{0}), ' ');
    std::cout << path << padding << " -> " << result.to_string() << "\n";

    if (result.type_index() == std::type_index{typeid(unsigned long)}) {
        return *std::bit_cast<unsigned long*>(result.buffer());
    }
    return 0;
}

void map_all(libtokamap::MappingHandler& mapping_handler, const std::string& mapping)
{
    map(mapping_handler, mapping, "magnetics/version");
    auto num_coils = map(mapping_handler, mapping, "magnetics/coil");
    for (auto coil = 0UL; coil < num_coils; ++coil) {
        std::string path = "magnetics/coil[" + std::to_string(coil) + "]";
        map(mapping_handler, mapping, path + "/name");
        auto num_positions = map(mapping_handler, mapping, path + "/position");
        for (auto position = 0UL; position < num_positions; ++position) {
            std::string pos_path = path + ("/position[" + std::to_string(position) + "]");
            map(mapping_handler, mapping, pos_path + "/r");
            map(mapping_handler, mapping, pos_path + "/z");
        }
        map(mapping_handler, mapping, path + "/flux/time");
        map(mapping_handler, mapping, path + "/flux/data");
    }
}
} // namespace

int main()
{
    try {
        libtokamap::MappingHandler mapping_handler;

        auto root = std::filesystem::path{__FILE__}.parent_path().parent_path();

        std::filesystem::path data_root = root / "data";
        auto data_source = std::make_unique<JSONDataSource>(data_root);
        libtokamap::DataSourceMapping::register_data_source("JSON", std::move(data_source));

        auto schema_root = root.parent_path().parent_path() / "schemas";
        nlohmann::json config = {{"mapping_directory", (root / "mappings").string()},
                                 {"mapping_schema", (schema_root / "mappings.schema.json").string()},
                                 {"globals_schema", (schema_root / "globals.schema.json").string()},
                                 {"mapping_config_schema", (schema_root / "mappings.cfg.schema.json").string()}};
        mapping_handler.init(config);

        const char* mapping = "EXAMPLE";
        map_all(mapping_handler, mapping);
    } catch (std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
