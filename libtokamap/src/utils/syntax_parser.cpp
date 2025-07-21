#include "syntax_parser.hpp"

#include <ctre/ctre.hpp>
#include <nlohmann/json.hpp>
#include <stack>
#include <string>
#include <string_view>

using namespace std::string_literals;

namespace
{

constexpr auto indices_re = ctll::fixed_string{R"(\{\{\s*(#\d+|.*\[#\d+\](\.\S+)?)\s*\}\})"};
constexpr auto simple_index_re = ctll::fixed_string{R"(#(\d+))"};
constexpr auto array_index_re = ctll::fixed_string{R"((.*)\[#(\d+)\](\.\S+)?)"};
constexpr auto subindices_re = ctll::fixed_string{R"(\((.*\[#(\d+)\](\.\S+))\))"};

std::string expand_indices(const std::string& input)
{
    // Handle simple index directly
    if (auto match = ctre::match<simple_index_re>(input)) {
        return "indices." + match.get<1>().to_string();
    }

    // Handle array index with possible nesting
    if (auto match = ctre::match<array_index_re>(input)) {
        std::string array = match.get<1>().to_string();
        const std::string_view index = match.get<2>();
        const std::string_view field = match.get<3>();

        // Iteratively unwrap nested array indices
        while (true) {
            if (auto submatch = ctre::match<subindices_re>(array)) {
                array = submatch.get<1>().to_string();
            } else if (auto nested_array = ctre::match<array_index_re>(array)) {
                const std::string_view subarray = nested_array.get<1>();
                const std::string_view subindex = nested_array.get<2>();
                const std::string subfield = nested_array.get<3>().to_string();

                array = "at("s.append(subarray).append(", indices.").append(subindex).append(")");
                if (!subfield.empty()) {
                    array += subfield;
                }
            } else {
                break;
            }
        }

        std::string result = "at("s.append(array).append(", indices.").append(index).append(")");
        if (!field.empty()) {
            result += field;
        }
        return result;
    }

    // Fallback if no match
    return input;
}

std::string process_string_node(std::string value) {
    std::string result;
    auto iter = value.begin();

    for (const auto& match : ctre::search_all<indices_re>(value)) {
        std::string prefix{iter, match.begin()};
        auto expression = match.get<1>().to_string();
        result.append(prefix).append("{{ ").append(expand_indices(expression)).append(" }}");
        iter = match.end();
    }

    result.append(std::string{iter, value.end()});
    return result;
}

void walk_json(nlohmann::json& root)
{
    std::stack<nlohmann::json*> stack;
    stack.push(&root);

    while (!stack.empty()) {
        nlohmann::json* current = stack.top();
        stack.pop();

        for (const auto& element : current->items()) {
            if (element.key() == "MAP_TYPE") {
                continue; // Skip MAP_TYPE elements
            }

            auto& node = element.value();
            if (node.is_string()) {
                node = process_string_node(node);
            } else if (node.is_object()) {
                stack.push(&node);
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

    return input;
}
