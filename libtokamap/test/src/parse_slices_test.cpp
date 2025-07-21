#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <cstdlib>

#include "utils/subset.hpp"

using namespace libtokamap;

TEST_CASE("Parsing of slice strings", "[slice]") {

    constexpr size_t dim_size = 10;

    SECTION("select single element") {
        std::vector<size_t> shape = {dim_size};
        auto slices = libtokamap::parse_slices("[3]", shape);

        REQUIRE(slices.size() == 1);
        REQUIRE(slices[0].start() == 3);
        REQUIRE(slices[0].stop() == 4);
        REQUIRE(slices[0].stride() == 1);
    }

    SECTION("select single element outside of range throws exception") {
        std::vector<size_t> shape = {dim_size};
        REQUIRE_THROWS(libtokamap::parse_slices("[11]", shape));
    }

    SECTION("select all of dimension") {
        std::vector<size_t> shape = {dim_size};
        auto slices = libtokamap::parse_slices("[:]", shape);

        REQUIRE(slices.size() == 1);
        REQUIRE(slices[0].start() == 0);
        REQUIRE(slices[0].stop() == dim_size);
        REQUIRE(slices[0].stride() == 1);
    }

    SECTION("select range of dimension") {
        std::vector<size_t> shape = {dim_size};
        auto slices = libtokamap::parse_slices("[3:7]", shape);

        REQUIRE(slices.size() == 1);
        REQUIRE(slices[0].start() == 3);
        REQUIRE(slices[0].stop() == 7);
        REQUIRE(slices[0].stride() == 1);
    }

    SECTION("select range of dimension using negative indices") {
        std::vector<size_t> shape = {dim_size};
        auto slices = libtokamap::parse_slices("[3:-2]", shape);

        REQUIRE(slices.size() == 1);
        REQUIRE(slices[0].start() == 3);
        REQUIRE(slices[0].stop() == 9);
        REQUIRE(slices[0].stride() == 1);
    }

    SECTION("select all with stride") {
        std::vector<size_t> shape = {dim_size};
        auto slices = libtokamap::parse_slices("[::2]", shape);

        REQUIRE(slices.size() == 1);
        REQUIRE(slices[0].start() == 0);
        REQUIRE(slices[0].stop() == dim_size);
        REQUIRE(slices[0].stride() == 2);
    }

    SECTION("select range with stride") {
        std::vector<size_t> shape = {dim_size};
        auto slices = libtokamap::parse_slices("[3:7:2]", shape);

        REQUIRE(slices.size() == 1);
        REQUIRE(slices[0].start() == 3);
        REQUIRE(slices[0].stop() == 7);
        REQUIRE(slices[0].stride() == 2);
    }

    SECTION("select range with negative stride") {
        std::vector<size_t> shape = {dim_size};
        auto slices = libtokamap::parse_slices("[3:7:-1]", shape);

        REQUIRE(slices.size() == 1);
        REQUIRE(slices[0].start() == 3);
        REQUIRE(slices[0].stop() == 7);
        REQUIRE(slices[0].stride() == -1);
    }

    SECTION("select too large range throws exception") {
        std::vector<size_t> shape = {dim_size};
        REQUIRE_THROWS(libtokamap::parse_slices("[3:11]", shape));
    }

    SECTION("select all for each dimension") {
        std::vector<size_t> shape = {dim_size, dim_size, dim_size};
        auto slices = libtokamap::parse_slices("[:][:][:]", shape);

        REQUIRE(slices.size() == 3);

        REQUIRE(slices[0].start() == 0);
        REQUIRE(slices[0].stop() == 10);
        REQUIRE(slices[0].stride() == 1);

        REQUIRE(slices[1].start() == 0);
        REQUIRE(slices[1].stop() == 10);
        REQUIRE(slices[1].stride() == 1);

        REQUIRE(slices[2].start() == 0);
        REQUIRE(slices[2].stop() == 10);
        REQUIRE(slices[2].stride() == 1);
    }

    SECTION("select range for each dimension") {
        std::vector<size_t> shape = {dim_size, dim_size, dim_size};
        auto slices = libtokamap::parse_slices("[1:3][2:4][3:5]", shape);

        REQUIRE(slices.size() == 3);

        REQUIRE(slices[0].start() == 1);
        REQUIRE(slices[0].stop() == 3);
        REQUIRE(slices[0].stride() == 1);

        REQUIRE(slices[1].start() == 2);
        REQUIRE(slices[1].stop() == 4);
        REQUIRE(slices[1].stride() == 1);

        REQUIRE(slices[2].start() == 3);
        REQUIRE(slices[2].stop() == 5);
        REQUIRE(slices[2].stride() == 1);
    }

    SECTION("selecting more ranges than dimensions throws exception") {
        std::vector<size_t> shape = {dim_size, dim_size};
        REQUIRE_THROWS(libtokamap::parse_slices("[1:3][2:4][3:5]", shape));
    }
}
