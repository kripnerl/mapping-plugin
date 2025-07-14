#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// UDA includes
#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <plugins/pluginStructs.h>
#include <server/getServerEnvironment.h>

#include "utils/print_uda_structs.hpp"

namespace json_mapping::subset
{

class SubsetInfo
{
  private:
    uint64_t _start;
    uint64_t _stop;
    int64_t _stride = 1;
    uint64_t _dim_size;

  public:
    explicit SubsetInfo(uint64_t size) : _start(0), _stop(size), _dim_size(size) {}

    SubsetInfo(uint64_t start, uint64_t stop, int stride, uint64_t size)
        : _start(start), _stop(stop), _stride(stride), _dim_size(size)
    {

        if (start == 0 and stop == 0) {
            _stop = size;
        };
    }

    [[nodiscard]] uint64_t size() const { return std::floor((_stop - _start) / _stride); }

    [[nodiscard]] bool validate() const { return _stop <= _dim_size and _stride < _dim_size; }

    [[nodiscard]] uint64_t start() const { return _start; }

    [[nodiscard]] uint64_t stop() const { return _stop; }

    [[nodiscard]] int64_t stride() const { return _stride; }

    [[nodiscard]] uint64_t dim_size() const { return _dim_size; }

    [[nodiscard]] std::string print_to_string() const
    {
        std::stringstream out;
        out << "start: " << _start << "\n";
        out << "stop: " << _stop << "\n";
        out << "stride: " << _stride << "\n";
        out << "dim_size: " << _dim_size << "\n";
        return out.str();
    }
};

void apply_subsetting(DATA_BLOCK* data_block, SUBSET& data_subset, double scale_factor, double offset);

template <typename T> void do_subset(DATA_BLOCK* data_block, SUBSET& data_subset, double scale_factor, double offset);

template <typename T> void do_dim_subset(DIMS* dim, const SubsetInfo& subset_info, double scale_factor, double offset);

void apply_dim_subsetting(DIMS* dim, const SubsetInfo& subset_info, double scale_factor, double offset);

std::vector<SubsetInfo> subset_info_converter(const SUBSET& datasubset, const DATA_BLOCK* data_block);

template <typename T>
std::vector<T> subset(std::vector<T>& input, std::vector<SubsetInfo>& subset_dims, double scale_factor = 1.0,
                      double offset = 0.0);

void collapse_dims(DATA_BLOCK* data_block, std::vector<SubsetInfo>& subset_dims);

enum class LogLevel : uint8_t { DEBUG, INFO, WARNING, ERROR };

inline int log(LogLevel log_level, std::string_view log_msg)
{
    const char* log_env_option = getenv("UDA_JSON_MAPPING_SUBSET_LOGGING");
    if (log_env_option == nullptr or std::stoi(log_env_option) <= 0) {
        return 0;
    }

    const ENVIRONMENT* environment = getServerEnvironment();

    std::string const log_file_name = std::string{static_cast<const char*>(environment->logdir)} + "/subset.log";
    std::ofstream log_file;
    log_file.open(log_file_name, std::ios_base::out | std::ios_base::app);
    std::time_t const time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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

inline void log_request_status(REQUEST_DATA* request_data, const std::string& message)
{
    log(LogLevel::DEBUG, message + "\n" + uda_structs::print_request_data(request_data));
}

} // namespace json_mapping::subset
