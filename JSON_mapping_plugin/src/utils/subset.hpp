#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <optional>
#include <string>
#include <string_view>

#include "map_types/map_arguments.hpp"

namespace json_mapping::subset
{

void update_array(TypedDataArray& input, const std::optional<std::string>& slice, std::optional<float> scale_factor,
                  std::optional<float> offset);

enum class LogLevel : uint8_t { DEBUG, INFO, WARNING, ERROR };

inline int log(LogLevel log_level, std::string_view log_msg)
{
    const char* log_env_option = getenv("JSON_MAPPING_SUBSET_LOGGING");
    if (log_env_option == nullptr or std::stoi(log_env_option) <= 0) {
        return 0;
    }

    std::string log_dir;

    const std::string log_file_name = log_dir + "/subset.log";
    std::ofstream log_file;
    log_file.open(log_file_name, std::ios_base::out | std::ios_base::app);
    const std::time_t time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto timestamp = std::put_time(std::gmtime(&time_now), "%Y-%m-%d:%H:%M:%S"); // NOLINT(concurrency-mt-unsafe)
    if (!log_file) {
        return 1;
    }

    switch (log_level) {
        case LogLevel::DEBUG:
            log_file << timestamp << ":DEBUG - ";
            break;
        case LogLevel::INFO:
            log_file << timestamp << ":INFO - ";
            break;
        case LogLevel::WARNING:
            log_file << timestamp << ":WARNING - ";
            break;
        case LogLevel::ERROR:
            log_file << timestamp << ":ERROR - ";
            break;
        default:
            log_file << "LOG_LEVEL NOT DEFINED";
    }
    log_file << log_msg << "\n";
    log_file.close();

    return 0;
}

} // namespace json_mapping::subset
