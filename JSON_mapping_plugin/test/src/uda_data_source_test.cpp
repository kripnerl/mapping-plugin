#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm_ext/iota.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

#include <clientserver/initStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/pluginStructs.h>

#include "map_types/data_source_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

#include "test_helpers.hpp"
#include "uda/uda_data_source.hpp"

using namespace json_mapping;

static int plugin_return_scalar(IDAM_PLUGIN_INTERFACE* interface)
{
    REQUIRE(std::string{interface->request_data->signal} == "get()");
    DATA_BLOCK* data_block = interface->data_block;
    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_INT;
    data_block->data_n = 1;
    data_block->rank = 0;
    data_block->data = static_cast<char*>(malloc(sizeof(int)));
    reinterpret_cast<int*>(data_block->data)[0] = 42;
    return 0;
}

constexpr size_t array_size = 100;

static int plugin_return_array(IDAM_PLUGIN_INTERFACE* interface)
{
    REQUIRE(std::string{interface->request_data->signal} == "get()");
    DATA_BLOCK* data_block = interface->data_block;
    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_FLOAT;
    data_block->data_n = array_size;
    data_block->rank = 1;
    data_block->dims = static_cast<Dims*>(malloc(sizeof(Dims)));
    data_block->dims[0].dim_n = array_size;
    data_block->data = static_cast<char*>(malloc(sizeof(float) * array_size));
    for (size_t i = 0; i < array_size; ++i) {
        reinterpret_cast<float*>(data_block->data)[i] = static_cast<float>(i);
    }
    return 0;
}

TEST_CASE("PluginMapping calls UDA data source", "[plugin_mapping][uda_data_source]")
{
    PLUGINLIST plugin_list = {0};
    plugin_list.count = 1;
    plugin_list.mcount = 1;

    PLUGIN_DATA plugin = {0};
    plugin.idamPlugin = &plugin_return_scalar;
    plugin_list.plugin = &plugin;
    std::strcpy(plugin.format, "UDA");

    auto test_source = std::make_unique<UDADataSource>("UDA", "get", &plugin_list, false);
    DataSourceMapping::register_data_source("UDA", std::move(test_source));

    SECTION("Integer values are correctly returned")
    {
        DataSourceArgs request_args = {};
        std::optional<float> offset = {};
        std::optional<float> scale = {};
        std::optional<std::string> slice = {};
        std::shared_ptr<ram_cache::RamCache> ram_cache = nullptr;

        auto mapping = std::make_unique<DataSourceMapping>("UDA", request_args, offset, scale, slice, ram_cache);
        REQUIRE(mapping != nullptr);

        MapArguments map_args = makeMapArguments(std::type_index{typeid(int)}, 1);
        auto array = mapping->map(map_args); // FIXME

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(int)});
        REQUIRE(array.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(array.buffer()) == 42);
    }
}

TEST_CASE("Slicing and offsetting returned data", "[plugin_mapping][uda_data_source]")
{
    PLUGINLIST plugin_list = {0};
    plugin_list.count = 1;
    plugin_list.mcount = 1;

    PLUGIN_DATA plugin = {0};
    plugin.idamPlugin = &plugin_return_array;
    plugin_list.plugin = &plugin;
    std::strcpy(plugin.format, "UDA");

    auto test_source = std::make_unique<UDADataSource>("UDA", "get", &plugin_list, false);
    DataSourceMapping::register_data_source("UDA", std::move(test_source));

    SECTION("Float values are correctly returned")
    {
        DataSourceArgs request_args = {};
        std::optional<float> offset = {};
        std::optional<float> scale = {};
        std::optional<std::string> slice = {};
        std::shared_ptr<ram_cache::RamCache> ram_cache = nullptr;

        auto mapping = std::make_unique<DataSourceMapping>("UDA", request_args, offset, scale, slice, ram_cache);
        REQUIRE(mapping != nullptr);

        MapArguments map_args = makeMapArguments(std::type_index{typeid(int)}, 1);
        auto array = mapping->map(map_args); // FIXME

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(float)});
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == array_size);
        REQUIRE(array.shape() == std::vector<size_t>{array_size});
        std::vector<float> expected(array_size);
        boost::range::iota(expected, 0);
        REQUIRE(array.span<float>() == gsl::span<float>{expected});
    }

    SECTION("Subset returned array")
    {
        DataSourceArgs request_args = {};
        constexpr float offset = 10.0;
        constexpr float scale = 2.0;
        constexpr size_t range_len = 10;
        std::string slice = "[0:" + std::to_string(range_len) + "]";
        std::shared_ptr<ram_cache::RamCache> ram_cache = nullptr;

        auto mapping = std::make_unique<DataSourceMapping>("UDA", request_args, offset, scale, slice, ram_cache);
        REQUIRE(mapping != nullptr);

        MapArguments map_args = makeMapArguments(std::type_index{typeid(int)}, 1);
        auto array = mapping->map(map_args); // FIXME

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(float)});
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == range_len);
        REQUIRE(array.shape() == std::vector<size_t>{range_len});
        std::vector<float> expected(range_len);
        boost::range::iota(expected, 0.0);
        boost::range::transform(expected, expected.begin(), [](float val) { return (val * scale) + offset; });
        REQUIRE(array.span<float>() == gsl::span<float>{expected});
    }
}
