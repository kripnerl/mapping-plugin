#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "utils/syntax_parser.hpp"

// @path => { "MAP_TYPE": "FORWARD", "VALUE": "path" }

TEST_CASE("Parse forward mapping", "[syntax_parser]") {
    SECTION("single element") {
        nlohmann::json input = "@FOO";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "FORWARD" },
            { "VALUE", "FOO" }
        };
        REQUIRE(result == expected);
    }

    SECTION("path") {
        nlohmann::json input = "@/A/B/C";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "FORWARD" },
            { "VALUE", "/A/B/C" }
        };
        REQUIRE(result == expected);
    }
}

TEST_CASE("Parse value mapping", "[syntax_parser]") {
    SECTION("string value") {
        nlohmann::json input = "foo";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "foo" }
        };
        REQUIRE(result == expected);
    }

    SECTION("integer value") {
        nlohmann::json input = 3;
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", 3 }
        };
        REQUIRE(result == expected);
    }

    SECTION("float value") {
        nlohmann::json input = 3.14;
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", 3.14 }
        };
        REQUIRE(result == expected);
    }
}

// walk for strings
// "{{ #X }}" => "{{ indices.X }}"
// "{{ A[#N].B }}" => "{{ at(A, indices.N).B }}"
// "{{ (A)[#N].B }}" => parse(A) + "[#N].B"

TEST_CASE("Parse indices expansion", "[syntax_parser]") {
    SECTION("simple index") {
        nlohmann::json input = "{{ #3 }}";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "{{ indices.3 }}" }
        };
        REQUIRE(result == expected);
    }

    //coils/#0/name
    SECTION("index in path") {
        nlohmann::json input = "foo/{{ #0 }}/bar";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "foo/{{ indices.0 }}/bar" }
        };
        REQUIRE(result == expected);
    }

    SECTION("multiple indices in path") {
        nlohmann::json input = "foo/{{ #0 }}/bar/{{ #1 }}/baz";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "foo/{{ indices.0 }}/bar/{{ indices.1 }}/baz" }
        };
        REQUIRE(result == expected);
    }

    SECTION("simple index as nested field") {
        nlohmann::json input = {
            { "MAP_TYPE", "PLUGIN" },
            { "ARGS", {
                { "signal", "{{ #0 }}" },
            } }
        };
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "PLUGIN" },
            { "ARGS", {
                { "signal", "{{ indices.0 }}" },
            } }
        };
        REQUIRE(result == expected);
    }

    // "{{ A[#N] }}" => "{{ at(A, indices.N) }}"
    SECTION("index into map/array") {
        nlohmann::json input = "{{ foo[#2] }}";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "{{ at(foo, indices.2) }}" }
        };
        REQUIRE(result == expected);
    }

    SECTION("index into map/array with field") {
        nlohmann::json input = "{{ foo[#1].bar }}";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "{{ at(foo, indices.1).bar }}" }
        };
        REQUIRE(result == expected);
    }

    SECTION("nested index into map/array") {
        nlohmann::json input = "{{ (foo[#0].bar)[#1] }}";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "{{ at(at(foo, indices.0).bar, indices.1) }}" }
        };
        REQUIRE(result == expected);
    }

    SECTION("nested index into map/array with field") {
        nlohmann::json input = "{{ (foo[#0].bar)[#1].baz }}";
        nlohmann::json result = libtokamap::parse(input);
        nlohmann::json expected = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", "{{ at(at(foo, indices.0).bar, indices.1).baz }}" }
        };
        REQUIRE(result == expected);
    }
}

TEST_CASE("indices expansion and bracket matching", "[process_string_node]") {
    SECTION("indices replace without syntactic sugar") {
        nlohmann::json input = "{{ #0 }}";
        nlohmann::json result = libtokamap::process_string_node(input);
        nlohmann::json expected = "{{ indices.0 }}";
        REQUIRE(result == expected);
    }
    SECTION("indices replace for arithmetic templating") {
        nlohmann::json input = "{{ #0 + 1 }}";
        nlohmann::json result = libtokamap::process_string_node(input);
        nlohmann::json expected = "{{ indices.0 + 1 }}";
        REQUIRE(result == expected);
    }
    SECTION("nested bracket replace (bracket matching)") {
        nlohmann::json input = "{{ foo[bar[#0].TYPE] }}";
        nlohmann::json result = libtokamap::process_string_node(input);
        nlohmann::json expected = "{{ at(foo, at(bar, indices.0).TYPE) }}";
        REQUIRE(result == expected);
    }
}
