#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include "map_types/value_mapping.hpp"
#include <clientserver/initStructs.h>
#include <nlohmann/json.hpp>
#include <memory>
#include "test_helpers.hpp"

TEST_CASE("ValueMapping can be constructed from JSON", "[value_mapping]") {
    // Setup test fixture
    nlohmann::json test_json = {
        {"MAP_TYPE", "VALUE"},
        {"VALUE", 42}
    };

    SECTION("Constructor works with integer value") {
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with float value") {
        test_json["VALUE"] = 3.14;
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with string value") {
        test_json["VALUE"] = "test_string";
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with array value") {
        test_json["VALUE"] = {1, 2, 3, 4, 5};
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with object value") {
        test_json["VALUE"] = {{"key1", "value1"}, {"key2", 2}};
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }
}

TEST_CASE("ValueMapping returns expected data for different 0D types", "[value_mapping_0D]") {

    auto* interface = PluginInterfaceFixture::Create();
    nlohmann::json test_json = {
        {"VALUE", 42}
    };


    SECTION("Integer values are correctly returned") {

        test_json["VALUE"] = 42;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_INT);
        REQUIRE(interface->data_block->rank == 0);
        REQUIRE(*reinterpret_cast<int*>(interface->data_block->data) == 42);
    }

    SECTION("Negative integer values are correctly returned") {

        test_json["VALUE"] = -42;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_INT);
        REQUIRE(interface->data_block->rank == 0);
        REQUIRE(*reinterpret_cast<int*>(interface->data_block->data) == -42);
    }

    SECTION("String values are correctly returned") {

        test_json["VALUE"] = "Hello World!";

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_STRING);
        REQUIRE(interface->data_block->rank == 1);
        REQUIRE_THAT(interface->data_block->data, Catch::Matchers::Equals("Hello World!"));
    }

    SECTION("Float values are correctly returned") {

        test_json["VALUE"] = 42.75;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_FLOAT);
        REQUIRE(interface->data_block->rank == 0);
        REQUIRE(*reinterpret_cast<float*>(interface->data_block->data) == 42.75);
    }

    SECTION("Negative float values are correctly returned") {

        test_json["VALUE"] = -42.75;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_FLOAT);
        REQUIRE(interface->data_block->rank == 0);
        REQUIRE(*reinterpret_cast<float*>(interface->data_block->data) == -42.75);
    }
}

TEST_CASE("ValueMapping returns expected data for different 1D types", "[value_mapping_1D]") {

    auto* interface = PluginInterfaceFixture::Create();
    nlohmann::json test_json = {
        {"VALUE", {0, 0, 0}}
    };

    SECTION("1D integer arrays are correctly returned") {

        std::vector<int> test_vector_1d{1, 2, 3, 4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_INT);
        REQUIRE(interface->data_block->rank == 1);
        const auto* data = reinterpret_cast<int*>(interface->data_block->data);
        REQUIRE_THAT(std::vector<int>(data, data + interface->data_block->data_n), Catch::Matchers::RangeEquals(test_vector_1d));
    }

    SECTION("1D negative integer arrays are correctly returned") {

        std::vector<int> test_vector_1d{-1, 2, -3, 4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_INT);
        REQUIRE(interface->data_block->rank == 1);
        const auto* data = reinterpret_cast<int*>(interface->data_block->data);
        REQUIRE_THAT(std::vector<int>(data, data + interface->data_block->data_n), Catch::Matchers::RangeEquals(test_vector_1d));
    }

    SECTION("1D float arrays are correctly returned") {

        std::vector<float> test_vector_1d{0.1, 0.2, 0.3, 0.4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_FLOAT);
        REQUIRE(interface->data_block->rank == 1);
        const auto* data = reinterpret_cast<float*>(interface->data_block->data);
        REQUIRE_THAT(std::vector<float>(data, data + interface->data_block->data_n), Catch::Matchers::RangeEquals(test_vector_1d));
    }

    SECTION("1D negative float arrays are correctly returned") {

        std::vector<float> test_vector_1d{0.1, -0.2, 0.3, -0.4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = makeMapArguments(interface);
        auto error_code = mapping->map(map_args);

        REQUIRE(error_code == 0);
        REQUIRE(interface->data_block != nullptr);
        REQUIRE(interface->data_block->data_type == UDA_TYPE_FLOAT);
        REQUIRE(interface->data_block->rank == 1);
        const auto* data = reinterpret_cast<float*>(interface->data_block->data);
        REQUIRE_THAT(std::vector<float>(data, data + interface->data_block->data_n), Catch::Matchers::RangeEquals(test_vector_1d));
    }
}

