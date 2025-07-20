#include "syntax_parser.hpp"

#include <boost/algorithm/string.hpp>
#include <ctre/ctre.hpp>
#include <nlohmann/json.hpp>
#include <string>

static constexpr auto indices_re = ctll::fixed_string{R"(\{\{\s*(#\d+|.*\[#\d+\](\.\S+)?)\s*\}\})"};
static constexpr auto simple_index_re = ctll::fixed_string{R"(#(\d+))"};
static constexpr auto array_index_re = ctll::fixed_string{R"((.*)\[#(\d+)\](\.\S+)?)"};
static constexpr auto subindices_re = ctll::fixed_string{R"(\((.*\[#(\d+)\](\.\S+))\))"};

namespace
{

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
        std::string result;
        result.append("at(").append(array).append(", indices.").append(index).append(")");
        if (!field.empty()) {
            result.append(field);
        }
        return result;
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
                std::string result;
                auto iter = value.begin();
                for (const auto& match : ctre::search_all<indices_re>(value)) {
                    std::string prefix{iter, match.begin()};
                    auto expression = match.get<1>().to_string();
                    result.append(prefix).append("{{ ").append(expand_indices(expression)).append(" }}");
                    iter = match.end();
                }
                result.append(std::string{iter, value.end()});
                element.value() = result;
            } else if (element.value().is_object()) {
                walk_json(element.value());
            }
        }
    }
}

} // namespace

nlohmann::json libtokamap::parse(nlohmann::json input)
{
    if (input.is_string()) {
        // parse forward mapping or simple string value
        std::string str = input;
        if (!str.empty() && str[0] == '@') {
            str = str.substr(1);
            input = {{"MAP_TYPE", "FORWARD"}, {"VALUE", str}};
        } else {
            input = {{"MAP_TYPE", "VALUE"}, {"VALUE", str}};
        }
    } else if (input.is_primitive()) {
        // parse simple non-string value
        input = {{"MAP_TYPE", "VALUE"}, {"VALUE", input}};
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
