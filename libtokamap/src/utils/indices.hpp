#pragma once

#include <utility>
#include <vector>
#include <deque>
#include <string>

namespace json_mapping {

std::pair<std::vector<int>, std::deque<std::string>>
extract_indices(const std::deque<std::string>& path_tokens);

} // namespace json_mapping
