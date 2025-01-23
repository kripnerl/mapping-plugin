#include "value_mapping.hpp"

#include <type_traits>
#include <stdexcept>
#include <vector>
#include <inja/inja.hpp>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>

#include "utils/uda_plugin_helpers.hpp"

using namespace inja;
using namespace nlohmann;

namespace {

template <typename T>
T string_to(const std::string& s, size_t* p);

template <> float string_to<float>(const std::string & s, size_t* p) { return std::stof(s, p); }
template <> double string_to<double>(const std::string & s, size_t* p) { return std::stod(s, p); }
template <> int32_t string_to<int32_t>(const std::string & s, size_t* p) { return std::stoi(s, p); }
template <> int64_t string_to<int64_t>(const std::string & s, size_t* p) { return std::stol(s, p); }

template <typename T>
std::string name();

template <> std::string name<float>() { return "float"; }
template <> std::string name<double>() { return "double"; }
template <> std::string name<int32_t>() { return "int32_t"; }
template <> std::string name<int64_t>() { return "int64_t"; }

template <typename T>
std::enable_if_t<std::is_scalar_v<T>, T>
try_convert(const std::string& input) {
    size_t end = 0;
    try {
        T result = string_to<T>(input, &end);
        if (end != input.size()) { throw std::invalid_argument("input is not a " + name<T>()); }
        return result;
    } catch (std::invalid_argument& e) {
        throw std::invalid_argument("input is not a " + name<T>());
    }
}

template <typename ARRAY_T>
std::enable_if_t<std::is_array_v<ARRAY_T>, std::vector<std::remove_all_extents_t<ARRAY_T>>>
try_convert(const std::string& input) {
    using T = std::remove_extent_t<ARRAY_T>;
    try {
        if (input.empty()) { throw std::invalid_argument("input is empty"); }
        if (input.front() != '[') { throw std::invalid_argument("input is not a " + name<T>() + "[]"); }
        if (input.back() != ']') { throw std::invalid_argument("input is not a " + name<T>() + "[]"); }

        auto trimmed = input.substr(1, input.size() - 2);

        std::vector<T> result;
        auto pos = trimmed.find(',');
        auto start = 0U;
        while (pos != std::string::npos)
        {
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

int type_deduce_array(DATA_BLOCK* data_block, const json& temp_val)
{
    switch (temp_val.front().type()) {
        case json::value_t::number_float: {
            // Handle array of floats
            auto temp_vec = temp_val.get<std::vector<float>>();
            imas_json_plugin::uda_helpers::set_return_data_array_vec<float>(data_block, temp_vec);
            break;
        }
        case json::value_t::number_integer: {
            // Handle array of ints
            auto temp_vec = temp_val.get<std::vector<int>>();
            imas_json_plugin::uda_helpers::set_return_data_array_vec<int>(data_block, temp_vec);
            break;
        }
        case json::value_t::number_unsigned: {
            // Handle array of ints
            auto temp_vec = temp_val.get<std::vector<unsigned int>>();
            imas_json_plugin::uda_helpers::set_return_data_array_vec<unsigned int>(data_block, temp_vec);
            break;
        }
        default:
            return 1;
    }

    return 0;
}

std::string render_string(const std::string& input, const json& global_data) {
    // Double inja template execution
    return render(render(input, global_data), global_data);
}

int type_deduce_primitive(DATA_BLOCK* data_block, const json& temp_val, const json& global_data, UDA_TYPE data_type, int rank)
{
    switch (temp_val.type()) {
        case json::value_t::number_float:
            // Handle float
            imas_json_plugin::uda_helpers::set_return_data_scalar_type<float>(data_block, temp_val.get<float>(), nullptr);
            break;
        case json::value_t::number_integer:
            // Handle int
            imas_json_plugin::uda_helpers::set_return_data_scalar_type<int>(data_block, temp_val.get<int>(), nullptr);
            break;
        case json::value_t::number_unsigned:
            // Handle int
            imas_json_plugin::uda_helpers::set_return_data_scalar_type<unsigned int>(data_block,
                                                                                 temp_val.get<unsigned int>(), nullptr);
            break;
        case json::value_t::boolean:
            // Handle bool
            imas_json_plugin::uda_helpers::set_return_data_scalar_type<bool>(data_block, temp_val.get<bool>(), nullptr);
            break;
        case json::value_t::string: {
            // Handle string
            std::string const rendered_string = render_string(temp_val.get<std::string>(), global_data);

            // try to convert to integer
            // catch exception - output as string
            // inja templating may replace with number
            try {
                if (rank == 0) {
                    switch (data_type) {
                        case UDA_TYPE_INT:
                            return imas_json_plugin::uda_helpers::set_return_data_scalar_type<int>(data_block, try_convert<int>(rendered_string), nullptr);
                        case UDA_TYPE_FLOAT:
                            return imas_json_plugin::uda_helpers::set_return_data_scalar_type<float>(data_block, try_convert<float>(rendered_string), nullptr);
                        case UDA_TYPE_DOUBLE:
                            return imas_json_plugin::uda_helpers::set_return_data_scalar_type<double>(data_block, try_convert<double>(rendered_string), nullptr);
                        default:
                            return setReturnDataString(data_block, rendered_string.c_str(), nullptr);
                    }
                } else {
                    switch (data_type) {
                        case UDA_TYPE_INT:
                            return imas_json_plugin::uda_helpers::set_return_data_array_vec<int>(data_block, try_convert<int[]>(rendered_string), nullptr);
                        case UDA_TYPE_FLOAT:
                            return imas_json_plugin::uda_helpers::set_return_data_array_vec<float>(data_block, try_convert<float[]>(rendered_string), nullptr);
                        case UDA_TYPE_DOUBLE:
                            return imas_json_plugin::uda_helpers::set_return_data_array_vec<double>(data_block, try_convert<double[]>(rendered_string), nullptr);
                        default:
                            return setReturnDataString(data_block, rendered_string.c_str(), nullptr);
                    }
                }
            } catch (const std::invalid_argument& e) {
                UDA_LOG(UDA_LOG_DEBUG,
                        "ValueMapping::map failure to convert"
                        "string to int in mapping : %s\n",
                        e.what());
                setReturnDataString(data_block, rendered_string.c_str(), nullptr);
            }
            break;
        }
        default:
            return 1;
    }

    return 0;
}

} // anon namespace

/**
 * @brief
 *
 * @param interface
 * @param entries
 * @param global_data
 * @param sig_type
 * @return
 */
int ValueMapping::map(const MapArguments& arguments) const
{

    const auto temp_val = m_value;
    if (temp_val.is_discarded() or temp_val.is_binary() or temp_val.is_null()) {
        UDA_LOG(UDA_LOG_DEBUG, "ValueMapping::map unrecognised json value type");
        return 1;
    }

    int err{1};
    if (temp_val.is_array()) {
        // Check all members of array are numbers
        // (Add array of strings if necessary)
        const bool all_number =
            std::all_of(temp_val.begin(), temp_val.end(), [](const json& els) { return els.is_number(); });

        // deduce type if true
        if (all_number) {
            err = type_deduce_array(arguments.m_interface->data_block, temp_val);
        }

    } else if (temp_val.is_primitive()) {
        err = type_deduce_primitive(arguments.m_interface->data_block, temp_val, arguments.m_global_data, arguments.m_datatype, arguments.m_rank);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "ValueMapping::map not structured or primitive");
    }

    return err;
}
