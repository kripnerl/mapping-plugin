#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <deque>
#include <string>

#include "utils/indices.hpp"

using namespace libtokamap;

TEST_CASE("Test extraction of indices", "[indices]") {

    SECTION("empty list of tokens") {
        auto [indices, tokens] = libtokamap::extract_indices({});

        REQUIRE(indices == std::vector<int>{});
        REQUIRE(tokens == std::deque<std::string>{});
    }

    SECTION("list containing empty path") {
        auto [indices, tokens] = libtokamap::extract_indices({""});

        REQUIRE(indices == std::vector<int>{});
        REQUIRE(tokens == std::deque<std::string>{""});
    }

    SECTION("single token containing only index") {
        auto [indices, tokens] = libtokamap::extract_indices({"[1]"});

        REQUIRE(indices == std::vector<int>{1});
        REQUIRE(tokens == std::deque<std::string>{"[#]"});
    }

    SECTION("single token ending with index") {
        auto [indices, tokens] = libtokamap::extract_indices({"foo[1]"});

        REQUIRE(indices == std::vector<int>{1});
        REQUIRE(tokens == std::deque<std::string>{"foo[#]"});
    }

    SECTION("single token starting with index") {
        auto [indices, tokens] = libtokamap::extract_indices({"[1]foo"});

        REQUIRE(indices == std::vector<int>{1});
        REQUIRE(tokens == std::deque<std::string>{"[#]foo"});
    }

    SECTION("single token with index in the middle") {
        auto [indices, tokens] = libtokamap::extract_indices({"foo[1]bar"});

        REQUIRE(indices == std::vector<int>{1});
        REQUIRE(tokens == std::deque<std::string>{"foo[#]bar"});
    }

    SECTION("multiple tokens") {
        auto [indices, tokens] = libtokamap::extract_indices({"foo[1]", "bar[2]", "baz[3]"});

        REQUIRE(indices == std::vector<int>{1, 2, 3});
        REQUIRE(tokens == std::deque<std::string>{"foo[#]", "bar[#]", "baz[#]"});
    }
}
