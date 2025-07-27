#pragma once

#include <cstddef>
#include <exprtk/exprtk.hpp>
#include <inja/inja.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

namespace libtokamap
{

/**
 * @class ExprMapping
 * @brief ExprMapping class to the hold the EXPR MAP_TYPE after parsing from the
 * JSON mapping file
 *
 * The class holds an expression std::string 'm_expr' for evaluation and
 * computation when mapping. The string is evaluated by the templating library
 * pantor/inja on object creation. 'm_parameters' holds the std::strings of the
 * variables needed to evaluate the expression, eg. X+Y requires knowledge or
 * data retrieval of X and Y.
 *
 * The evaluation and computation of the expression is done using the expression
 * toolkit library exprtk, this is done in the templated function 'eval_expr'.
 * The needed parameters are required to be entries in the JSON mapping file for
 * the current group. Retrieval of the data is done as if the mapping was being
 * retrieved regardless of the expression operation.
 *
 */
class ExprMapping : public Mapping
{
  public:
    ExprMapping() = delete;
    ExprMapping(std::string expr, std::unordered_map<std::string, std::string> parameters)
        : m_expr{std::move(expr)}, m_parameters{std::move(parameters)} {};

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_expr;
    std::unordered_map<std::string, std::string> m_parameters;

    template <typename T> [[nodiscard]] TypedDataArray eval_expr(const MapArguments& arguments) const;
};

/**
 * @brief Function to
 * (1) perform the evaulation and computation of the expression string
 * using the exprtk library
 * (2) output the data in the correct format to the data_block
 *
 * @tparam T expression parameters template type, in theory the expression can
 * be evaluated with any floating-point type. However this is currently
 * hard-coded to use float.
 *
 * @param out_interface IDAM_PLUGIN_INTERFACE for access to request and
 * data_block
 * @param entries unordered map of all mappings loaded for this experiment and
 * group
 * @param global_data global JSON object used in templating
 * @return int error_code
 */
template <typename T> TypedDataArray ExprMapping::eval_expr(const MapArguments& arguments) const
{
    exprtk::symbol_table<T> symbol_table;
    exprtk::expression<T> expression;
    exprtk::parser<T> parser;

    bool vector_expr{false};
    bool first_vec_param{true};
    size_t result_size{1};

    symbol_table.add_constants();
    for (const auto& [key, json_name] : m_parameters) {
        auto array = arguments.entries.at(json_name)->map(arguments);
        if (array.empty()) {
            return array;
        }

        const T* raw_data = std::bit_cast<const T*>(array.buffer());
        size_t data_size = array.size();

        if (data_size > 1) {
            symbol_table.add_vector(key, const_cast<T*>(raw_data), data_size);
            if (first_vec_param) {
                // use size of first vector parameter to define the size
                // should use maximum but assumes all vectors in calculation same size
                result_size = array.size();
                first_vec_param = false;
            }
            vector_expr = true;
        } else {
            symbol_table.add_variable(key, *const_cast<T*>(raw_data));
        }
    }

    std::vector<T> result(result_size);
    if (vector_expr) {
        symbol_table.add_vector("RESULT", result);
    } else {
        symbol_table.add_variable("RESULT", result.front());
    }
    expression.register_symbol_table(symbol_table);

    // replace patterns in expression if necessary, e.g. expression: RESULT:=X+Y
    std::string expr_string{"RESULT:=" + inja::render(m_expr, arguments.global_data)};
    parser.compile(expr_string, expression);

    // expression.value() executes the calculation and returns a value
    // however, result added to the expression to handle vector operations
    expression.value();
    if (vector_expr) {
        return TypedDataArray{result};
    }
    return TypedDataArray{result.at(0)};
}

} // namespace libtokamap
