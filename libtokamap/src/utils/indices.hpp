#pragma once

#include <utility>
#include <vector>
#include <deque>
#include <string>
#include <string_view>

namespace libtokamap {

std::pair<std::vector<int>, std::deque<std::string>>
extract_indices(const std::deque<std::string_view>& path_tokens);

} // namespace libtokamap
