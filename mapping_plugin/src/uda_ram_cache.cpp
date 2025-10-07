#include "uda_ram_cache.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// LibTokaMap includes
#include <utils/ram_cache.hpp>

// UDA includes
#include <clientserver/compressDim.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <server/getServerEnvironment.h>

#include "print_uda_structs.hpp"
#include "uda_type_sizes.hpp"

namespace
{

enum class LogLevel : uint8_t { DEBUG, INFO, WARNING, ERROR };

bool logging_active;

/**
 * @brief Temporary logging function for libtokamap_plugin, outputs
 * to UDA_HOME/etc/
 *
 * @param log_level The LogLevel (INFO, WARNING, ERROR, DEBUG)
 * @param log_msg The message to be logged
 * @return
 */
inline void log(LogLevel log_level, std::string_view log_msg)
{
    if (!logging_active) {
        return;
    }

    const ENVIRONMENT* environment = getServerEnvironment();

    std::string const log_file_name = std::string{static_cast<const char*>(environment->logdir)} + "/ramcache.log";
    std::ofstream log_file;
    log_file.open(log_file_name, std::ios_base::out | std::ios_base::app);
    std::time_t const time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto timestamp = std::put_time(std::gmtime(&time_now), "%Y-%m-%d:%H:%M:%S"); // NOLINT(concurrency-mt-unsafe)
    if (!log_file) {
        return;
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
}

inline void log_datablock_status(DATA_BLOCK* data_block, const std::string& message)
{
    if (!logging_active) {
        return;
    }
    log(LogLevel::DEBUG, message + "\n" + json_plugin::print_data_block(data_block));
}

void set_logging_option()
{
    const char* log_env_option = getenv("UDA_MAPPING_CACHE_LOGGING");
    logging_active = (log_env_option != nullptr) and (std::stoi(log_env_option) > 0);
}

std::unique_ptr<json_plugin::UDACacheEntry> make_data_entry(DATA_BLOCK* data_block)
{
    log_datablock_status(data_block, "data_block before caching");

    auto data_entry = std::make_unique<json_plugin::UDACacheEntry>();
    size_t byte_length = data_block->data_n * json_plugin::size_of_uda_type(data_block->data_type);
    data_entry->data.reserve(byte_length);
    std::copy(data_block->data, data_block->data + byte_length, std::back_inserter(data_entry->data));

    data_entry->dims.reserve(data_block->rank);
    for (unsigned int i = 0; i < data_block->rank; ++i) {
        auto dim = data_block->dims[i];

        // expand any compressed dims for caching.
        if (dim.compressed != 0) {
            uncompressDim(&dim);
            dim.compressed = 0;
            dim.method = 0;
            free(dim.sams);
            free(dim.offs);
            free(dim.ints);
            dim.udoms = 0;
            dim.sams = nullptr;
            dim.offs = nullptr;
            dim.ints = nullptr;
        }

        size_t dim_byte_length = dim.dim_n * json_plugin::size_of_uda_type(dim.data_type);
        std::vector<char> dim_vals(dim.dim, dim.dim + dim_byte_length);
        data_entry->dims.emplace_back(dim_vals);
        data_entry->dim_types.emplace_back(dim.data_type);
    }
    data_entry->order = data_block->order;
    data_entry->data_type = data_block->data_type;

    if (data_block->errhi != nullptr and data_block->error_type > 0) {
        size_t errhi_bytes = data_block->data_n * json_plugin::size_of_uda_type(data_block->error_type);
        data_entry->error_high.reserve(errhi_bytes);
        std::copy(data_block->errhi, data_block->errhi + errhi_bytes, std::back_inserter(data_entry->error_high));
        data_entry->error_type = data_block->error_type;
    }
    if (data_block->errlo != nullptr and data_block->error_type > 0) {
        size_t errlo_bytes = data_block->data_n * json_plugin::size_of_uda_type(data_block->error_type);
        data_entry->error_low.reserve(errlo_bytes);
        std::copy(data_block->errlo, data_block->errlo + errlo_bytes, std::back_inserter(data_entry->error_high));
        data_entry->error_type = data_block->error_type;
    }

    return data_entry;
}

} // namespace

void json_plugin::copy_to_cache(libtokamap::RamCache& ram_cache, const std::string& key, const DATA_BLOCK* data_block)
{
    auto entry = std::make_unique<UDACacheEntry>();

    size_t data_size = data_block->data_n * size_of_uda_type(data_block->data_type);
    entry->data.resize(data_size);
    std::copy(data_block->data, data_block->data + data_size, entry->data.begin());

    entry->error_high = {};
    entry->error_low = {};

    entry->dims.resize(data_block->rank);
    entry->dim_types.resize(data_block->rank);
    for (int i = 0; i < data_block->rank; i++) {
        const DIMS* dim = &data_block->dims[i];
        size_t dim_size = dim->dim_n * size_of_uda_type(dim->data_type);
        entry->dims[i].resize(dim_size);
        std::copy(dim->dim, dim->dim + dim_size, entry->dims[i].begin());
        entry->dim_types[i] = dim->data_type;
    }

    entry->order = data_block->order;
    entry->data_type = data_block->data_type;
    entry->error_type = data_block->error_type;

    ram_cache.add(key, std::move(entry));
}

bool json_plugin::copy_from_cache(const libtokamap::RamCache& cache, const std::string& key, DATA_BLOCK* data_block)
{
    auto entry = cache.get(key);
    if (!entry) {
        return false;
    }
    log(LogLevel::INFO, "key found in ramcache: \"" + key + "\". copying data out");

    auto* data_entry = dynamic_cast<UDACacheEntry*>(entry.value());
    if (data_entry == nullptr) {
        return false;
    }

    // DATA_BLOCK* data_block = (DATA_BLOCK*) malloc(sizeof(DATA_BLOCK));
    initDataBlock(data_block);
    data_block->data_type = data_entry->data_type;
    data_block->data_n = static_cast<int>(data_entry->data.size()) / size_of_uda_type(data_entry->data_type);

    log(LogLevel::INFO, "data size is: " + std::to_string(data_block->data_n));

    data_block->data = (char*)malloc(data_entry->data.size());
    std::copy(data_entry->data.begin(), data_entry->data.end(), data_block->data);

    if (!data_entry->error_high.empty()) {
        data_block->errhi = (char*)malloc(data_entry->error_high.size());
        std::copy(data_entry->error_high.begin(), data_entry->error_high.end(), data_block->errhi);
    }
    if (!data_entry->error_low.empty()) {
        data_block->errlo = (char*)malloc(data_entry->error_low.size());
        std::copy(data_entry->error_low.begin(), data_entry->error_low.end(), data_block->errlo);
    }

    data_block->rank = data_entry->dims.size();
    log(LogLevel::INFO, "data rank is: " + std::to_string(data_block->rank));

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; ++i) {
        DIMS* dim = &data_block->dims[i];
        initDimBlock(dim);

        dim->data_type = data_entry->dim_types[i];
        dim->dim_n = data_entry->dims[i].size() / size_of_uda_type(dim->data_type);

        log(LogLevel::INFO, "dim " + std::to_string(i) + " length: " + std::to_string(dim->dim_n));

        dim->dim = (char*)malloc(data_entry->dims[i].size());
        std::copy(data_entry->dims[i].begin(), data_entry->dims[i].end(), dim->dim);
    }

    data_block->order = data_entry->order;

    log_datablock_status(data_block, "data_block from cache");
    return true;
}
