#include "syntax_parser.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <boost/algorithm/string.hpp>
#include <ctre/ctre.hpp>
#include <string_view>

static constexpr auto indices_re = ctll::fixed_string{ R"(\{\{\s*(#\d+|.*\[#\d+\](\.\S+)?)\s*\}\})" };
static constexpr auto simple_index_re = ctll::fixed_string{ R"(#(\d+))" };
static constexpr auto array_index_re = ctll::fixed_string{ R"((.*)\[#(\d+)\](\.\S+)?)" };
static constexpr auto subindices_re = ctll::fixed_string{ R"(\((.*\[#(\d+)\](\.\S+))\))" };

namespace {

std::string expand_indices(const std::string& input)
{
    if (auto match = ctre::match<simple_index_re>(input)) {
        return "indices." + match.get<1>().to_string();
    }
    if (auto match = ctre::match<array_index_re>(input)) {
        std::string array = match.get<1>().to_string();
        std::string index = match.get<2>().to_string();
        std::string field = match.get<3>().to_string();
        if (auto submatch = ctre::match<subindices_re>(array)) {
            array = expand_indices(submatch.get<1>().to_string());
        }
        if (field.empty()) {
            return "at(" + array + ", indices." + index + ")";
        }
        return "at(" + array + ", indices." + index + ")" + field;
    }
    return input;
}

void walk_json(nlohmann::json& json)
{
    for (const auto& element : json.items()) {
        // skip MAP_TYPE elements
        if (element.key() != "MAP_TYPE") {
            if (element.value().is_string()) {
                std::string value = element.value();
                if (auto match = ctre::match<indices_re>(value)) {
                    auto expression = match.get<1>().to_string();
                    element.value() = "{{ " + expand_indices(expression) + " }}";
                }
            } else if (element.value().is_object()) {
                walk_json(element.value());
            }
        }
    }
}

} // anon namespace

nlohmann::json libtokamap::parse(nlohmann::json input)
{
    if (input.is_string()) {
        // parse forward mapping or simple string value
        std::string str = input;
        if (!str.empty() && str[0] == '@') {
            str = str.substr(1);
            input = {
                { "MAP_TYPE", "FORWARD" },
                { "VALUE", str }
            };
        } else {
            input = {
                { "MAP_TYPE", "VALUE" },
                { "VALUE", str }
            };
        }
    } else if (input.is_primitive()) {
        // parse simple non-string value
        input = {
            { "MAP_TYPE", "VALUE" },
            { "VALUE", input }
        };
    }

    // walk object looking for strings with #N
    walk_json(input);

    // @path => { "MAP_TYPE": "FORWARD", "VALUE": "path" }
    // X (number, string) => { "MAP_TYPE": "VALUE", "VALUE": X }

    // walk for strings
    // "{{ #N }}" => "{{ indices.N }}"
    // "{{ A[#N].B }}" => "{{ at(A, indices.N).B }}"
    // "{{ (A)[#N].B }}" => parse(A) + "[#N].B"

    return input;
}
