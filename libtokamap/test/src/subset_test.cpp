#include <catch2/catch_test_macros.hpp>

#include <utils/subset.hpp>
#include <vector>
#include <cstddef>

#include "map_types/map_arguments.hpp"

// void apply_subset(TypedDataArray& input,
//      std::optional<std::string> slice,
//      std::optional<float> scale_factor,
//      std::optional<float> offset);

constexpr size_t array_size = 100;

TEST_CASE("Test slice operations") {

    // SECTION("1d array data") {
    //     std::vector<float> data(array_size);
    //     boost::range::iota(data, 0.);
    //     std::vector<size_t> shape = {array_size};

    //     SECTION("select single element") {
    //         libtokamap::TypedDataArray array{data, shape, true};
    //         libtokamap::subset::apply_subset(array, "[1]", {}, {});
    //         REQUIRE(array.size() == 1);
    //         REQUIRE(array.shape() == std::vector<size_t>{});
    //     }

    //     SECTION("select simple range") {
    //         libtokamap::TypedDataArray array{data, shape, true};
    //         libtokamap::subset::apply_subset(array, "[0:10]", {}, {});
    //         REQUIRE(array.size() == 10);
    //         REQUIRE(array.shape() == std::vector<size_t>{10});
    //     }
    // }
}
