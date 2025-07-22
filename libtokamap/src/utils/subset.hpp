#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "map_types/map_arguments.hpp"

namespace libtokamap
{

std::vector<libtokamap::SubsetInfo> parse_slices(const std::string& slice, const std::vector<size_t>& shape);

void update_array(TypedDataArray& input, const std::optional<std::string>& slice, std::optional<float> scale_factor,
                  std::optional<float> offset);

} // namespace libtokamap
