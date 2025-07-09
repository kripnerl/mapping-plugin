#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include <nlohmann/json.hpp>

TEST_CASE("Dummy test to check CTest integration", "[dummy]") {
    REQUIRE(1 + 1 == 2);
}

TEST_CASE("DataBlockFixture creates valid data_block", "[fixture]") {
    // auto interface = PluginInterfaceFixture::Create("get", "test_machine", "test_ids", 12345);
    auto* data_block = DataBlockFixture::Create();
    REQUIRE(data_block != nullptr);
}