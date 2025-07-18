#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>

#include "map_types/data_source_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

using namespace libtokamap;

class TestDataSource : public libtokamap::DataSource
{
    TypedDataArray get(const DataSourceArgs& /*map_args*/, const MapArguments& /*arguments*/,
                       libtokamap::RamCache* /*ram_cache*/) override
    {
        return {};
    }
};

TEST_CASE("Creating new PluginMapping", "[plugin_mapping]")
{
    SECTION("Create mock data source")
    {
        auto test_source = std::make_unique<TestDataSource>();
        DataSourceMapping::register_data_source("TEST", std::move(test_source));

        DataSourceArgs request_args = {};
        std::optional<float> offset = {};
        std::optional<float> scale = {};
        std::optional<std::string> slice = {};
        std::optional<std::string> function = {};
        std::shared_ptr<libtokamap::RamCache> ram_cache = nullptr;
        auto mapping = std::make_unique<DataSourceMapping>("TEST", request_args, offset, scale, slice, ram_cache);
        REQUIRE(mapping != nullptr);
    }
}
