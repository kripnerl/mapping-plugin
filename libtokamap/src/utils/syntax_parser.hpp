#pragma once

#include <nlohmann/json.hpp>

namespace libtokamap {

nlohmann::json parse(nlohmann::json input);
std::string process_string_node(std::string value);

} // namespace libtokamap
