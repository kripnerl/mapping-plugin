#pragma once

#include "map_types/map_arguments.hpp"

namespace libtokamap::map_transform
{

int transform_offset(TypedDataArray& array, float offset);
int transform_scale(TypedDataArray& array, float scale);

} // namespace libtokamap::map_transform
