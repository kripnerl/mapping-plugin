#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <clientserver/initStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <cstddef>
#include <cstring>
#include <plugins/pluginStructs.h>

#include <nlohmann/json.hpp>
#include <memory>
#include <optional>
#include <string>
#include <cstdlib>

#include "test_helpers.hpp"
#include "map_types/plugin_mapping.hpp"
#include "utils/ram_cache.hpp"
#include "map_types/map_arguments.hpp"

using namespace json_mapping;

TEST_CASE("Creating new PluginMapping", "[plugin_mapping]") {

    SECTION("Constructor works") {
        MapArgs_t request_args = {};
        std::optional<float> offset = {};
        std::optional<float> scale = {};
        std::optional<std::string> slice = {};
        std::optional<std::string> function = {};
        std::shared_ptr<ram_cache::RamCache> ram_cache = nullptr;
        const PLUGINLIST* plugin_list = nullptr;
        auto mapping = std::make_unique<PluginMapping>("UDA", request_args, offset, scale, slice, function, ram_cache, plugin_list);
        REQUIRE(mapping != nullptr);
    }

}

static int plugin_mock(IDAM_PLUGIN_INTERFACE* interface) {
    REQUIRE(std::string{ interface->request_data->signal } == "get()");
    DATA_BLOCK* data_block = interface->data_block;
    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_INT;
    data_block->data_n = 1;
    data_block->rank = 0;
    data_block->data = static_cast<char*>(malloc(sizeof(int)));
    reinterpret_cast<int*>(data_block->data)[0] = 42;
    return 0;
}

TEST_CASE("PluginMapping calls plugin", "[plugin_mapping]") {

    SECTION("Integer values are correctly returned") {

        MapArgs_t request_args = {};
        std::optional<float> offset = {};
        std::optional<float> scale = {};
        std::optional<std::string> slice = {};
        std::optional<std::string> function = {};
        std::shared_ptr<ram_cache::RamCache> ram_cache = nullptr;

        PLUGINLIST plugin_list = {0};
        plugin_list.count = 1;
        plugin_list.mcount = 1;

        PLUGIN_DATA plugin = {0};
        plugin.idamPlugin = &plugin_mock;
        plugin_list.plugin = &plugin;
        std::strcpy(plugin.format, "UDA");

        auto mapping = std::make_unique<PluginMapping>("UDA", request_args, offset, scale, slice, function, ram_cache, &plugin_list);
        REQUIRE(mapping != nullptr);

        MapArguments map_args = makeMapArguments(UDA_TYPE_FLOAT, 1);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{ typeid(int) });
        REQUIRE(array.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(array.buffer()) == 42);
    }

}
