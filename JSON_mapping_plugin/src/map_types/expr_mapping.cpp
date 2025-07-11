#include "map_types/expr_mapping.hpp"

#include "map_types/map_arguments.hpp"

// template int ExprMapping::eval_expr<float>(const MapArguments& arguments) const;
template int ExprMapping::eval_expr<double>(const MapArguments& arguments) const;

/**
 * @brief Entry map function, overriden from parent Mapping class
 *
 * @note expression is only of float type for testing purposes
 * @param interface IDAM_PLUGIN_INTERFACE for access to request and data_block
 * @param entries unordered map of all mappings loaded for this experiment and
 * IDS
 * @param global_data global JSON object used in templating
 * @return int error_code
 */
int ExprMapping::map(const MapArguments& arguments) const
{

    // Float only currently for testing purposes
    return eval_expr<double>(arguments);
};
