#include "value_mapping.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <inja/inja.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"

using namespace inja;
using namespace nlohmann;

namespace
{

template <typename T> T string_to(const std::string& string, size_t* ptr);

template <> float string_to<float>(const std::string& string, size_t* ptr) { return std::stof(string, ptr); }
template <> double string_to<double>(const std::string& string, size_t* ptr) { return std::stod(string, ptr); }
template <> int32_t string_to<int32_t>(const std::string& string, size_t* ptr) { return std::stoi(string, ptr); }
// template <> int64_t string_to<int64_t>(const std::string& string, size_t* ptr) { return std::stol(string, ptr); }

template <typename T> std::string name();

template <> std::string name<float>() { return "float"; }
template <> std::string name<double>() { return "double"; }
template <> std::string name<int32_t>() { return "int32_t"; }
// template <> std::string name<int64_t>() { return "int64_t"; }

template <typename T> std::enable_if_t<std::is_scalar_v<T>, T> try_convert(const std::string& input)
{
    size_t end = 0;
    try {
        T result = string_to<T>(input, &end);
        if (end != input.size()) {
            throw std::invalid_argument("input is not a " + name<T>());
        }
        return result;
    } catch (std::invalid_argument& e) {
        throw std::invalid_argument("input is not a " + name<T>());
    }
}

template <typename ARRAY_T>
std::enable_if_t<std::is_array_v<ARRAY_T>, std::vector<std::remove_all_extents_t<ARRAY_T>>>
try_convert(const std::string& input)
{
    using T = std::remove_extent_t<ARRAY_T>;
    try {
        if (input.empty()) {
            throw std::invalid_argument("input is empty");
        }
        if (input.front() != '[') {
            throw std::invalid_argument("input is not a " + name<T>() + "[]");
        }
        if (input.back() != ']') {
            throw std::invalid_argument("input is not a " + name<T>() + "[]");
        }

        auto trimmed = input.substr(1, input.size() - 2);

        std::vector<T> result;
        auto pos = trimmed.find(',');
        auto start = 0U;
        while (pos != std::string::npos) {
            auto sub = trimmed.substr(start, pos - start);
            start = pos + 1;
            pos = trimmed.find(',', start);
            result.emplace_back(try_convert<T>(sub));
        }
        auto rem = trimmed.substr(start);
        result.emplace_back(try_convert<T>(rem));
        return result;
    } catch (std::invalid_argument& e) {
        throw std::invalid_argument("input is not a " + name<T>() + "[]");
    }
}

libtokamap::TypedDataArray type_deduce_array(const json& temp_val)
{
    switch (temp_val.front().type()) {
        case json::value_t::number_float:
            return libtokamap::TypedDataArray{temp_val.get<std::vector<float>>()};
        case json::value_t::number_integer:
            return libtokamap::TypedDataArray{temp_val.get<std::vector<int>>()};
        case json::value_t::number_unsigned:
            return libtokamap::TypedDataArray{temp_val.get<std::vector<unsigned int>>()};
        default:
            return {};
    }
}

std::string render_string(const std::string& input, const json& global_data)
{
    // Double inja template execution
    return render(render(input, global_data), global_data);
}

libtokamap::TypedDataArray type_deduce_primitive(const json& temp_val, const json& global_data,
                                                 std::type_index data_type, int rank)
{
    switch (temp_val.type()) {
        case json::value_t::number_float:
            return libtokamap::TypedDataArray{temp_val.get<float>()};
        case json::value_t::number_integer:
            return libtokamap::TypedDataArray{temp_val.get<int>()};
        case json::value_t::number_unsigned:
            return libtokamap::TypedDataArray{temp_val.get<unsigned int>()};
        case json::value_t::boolean:
            return libtokamap::TypedDataArray{temp_val.get<bool>()};
        case json::value_t::string: {
            // Handle string
            std::string const rendered_string = render_string(temp_val.get<std::string>(), global_data);

            // try to convert to integer
            // catch exception - output as string
            // inja templating may replace with number
            try {
                if (rank == 0) {
                    using libtokamap::DataType;
                    switch (libtokamap::type_index_map(data_type)) {
                        case DataType::Int:
                            return libtokamap::TypedDataArray{try_convert<int>(rendered_string)};
                        case DataType::Float:
                            return libtokamap::TypedDataArray{try_convert<float>(rendered_string)};
                        case DataType::Double:
                            return libtokamap::TypedDataArray{try_convert<double>(rendered_string)};
                        default:
                            return libtokamap::TypedDataArray{rendered_string};
                    }
                } else {
                    using libtokamap::DataType;
                    switch (libtokamap::type_index_map(data_type)) {
                        case DataType::Int:
                            return libtokamap::TypedDataArray{try_convert<int[]>(rendered_string)};
                        case DataType::Float:
                            return libtokamap::TypedDataArray{try_convert<float[]>(rendered_string)};
                        case DataType::Double:
                            return libtokamap::TypedDataArray{try_convert<double[]>(rendered_string)};
                        default:
                            return libtokamap::TypedDataArray{rendered_string};
                    }
                }
            } catch (const std::invalid_argument& e) {
                // UDA_LOG(UDA_LOG_DEBUG,
                //         "ValueMapping::map failure to convert"
                //         "string to int in mapping : %s\n",
                //         e.what());
                return libtokamap::TypedDataArray{rendered_string};
            }
            break;
        }
        default:
            throw libtokamap::JsonError{"unknown json type"};
    }

    return {};
}

} // namespace

libtokamap::TypedDataArray libtokamap::ValueMapping::map(const MapArguments& arguments) const
{
    const auto temp_val = m_value;
    if (temp_val.is_discarded() or temp_val.is_binary() or temp_val.is_null()) {
        throw libtokamap::JsonError{"map unrecognised json value type"};
    }

    if (temp_val.is_array()) {
        // Check all members of array are numbers
        // (Add array of strings if necessary)
        const bool all_number =
            std::all_of(temp_val.begin(), temp_val.end(), [](const json& els) { return els.is_number(); });

        // deduce type if true
        if (all_number) {
            return type_deduce_array(temp_val);
        }

    } else if (temp_val.is_primitive()) {
        return type_deduce_primitive(temp_val, arguments.global_data, arguments.data_type, arguments.rank);
    } else {
        throw libtokamap::ProcessingError{"map not structured or primitive"};
    }

    return {};
}
