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
#include <vector>

#include <clientserver/compressDim.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <server/getServerEnvironment.h>

#include "utils/print_uda_structs.hpp"
#include "utils/ram_cache.hpp"
#include "utils/uda_type_sizes.hpp"

using ram_cache::uda::UDACacheEntry;
using uda_type_utils::size_of_uda_type;

namespace
{

enum class LogLevel : uint8_t { DEBUG, INFO, WARNING, ERROR };

bool logging_active;

/**
 * @brief Temporary logging function for JSON_mapping_plugin, outputs
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
    log(LogLevel::DEBUG, message + "\n" + uda_structs::print_data_block(data_block));
}

void set_logging_option()
{
    const char* log_env_option = getenv("UDA_JSON_MAPPING_CACHE_LOGGING");
    logging_active = (log_env_option != nullptr) and (std::stoi(log_env_option) > 0);
}

std::unique_ptr<ram_cache::uda::UDACacheEntry> make_data_entry(DATA_BLOCK* data_block)
{

    log_datablock_status(data_block, "data_block before caching");

    auto data_entry = std::make_unique<UDACacheEntry>();
    size_t byte_length = data_block->data_n * size_of_uda_type(data_block->data_type);
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

        size_t dim_byte_length = dim.dim_n * size_of_uda_type(dim.data_type);
        std::vector<char> dim_vals(dim.dim, dim.dim + dim_byte_length);
        data_entry->dims.emplace_back(dim_vals);
        data_entry->dim_types.emplace_back(dim.data_type);
    }
    data_entry->order = data_block->order;
    data_entry->data_type = data_block->data_type;

    if (data_block->errhi != nullptr and data_block->error_type > 0) {
        size_t errhi_bytes = data_block->data_n * size_of_uda_type(data_block->error_type);
        data_entry->error_high.reserve(errhi_bytes);
        std::copy(data_block->errhi, data_block->errhi + errhi_bytes, std::back_inserter(data_entry->error_high));
        data_entry->error_type = data_block->error_type;
    }
    if (data_block->errlo != nullptr and data_block->error_type > 0) {
        size_t errlo_bytes = data_block->data_n * size_of_uda_type(data_block->error_type);
        data_entry->error_low.reserve(errlo_bytes);
        std::copy(data_block->errlo, data_block->errlo + errlo_bytes, std::back_inserter(data_entry->error_high));
        data_entry->error_type = data_block->error_type;
    }

    return data_entry;
}

} // namespace

bool ram_cache::uda::copy_data_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block)
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

    initDataBlock(data_block);
    data_block->data_type = data_entry->data_type;
    data_block->data_n = data_entry->data.size() / size_of_uda_type(data_entry->data_type);

    log(LogLevel::INFO, "data size is: " + std::to_string(data_block->data_n));

    data_block->data = (char*)malloc(data_entry->data.size());
    std::copy(data_entry->data.data(), data_entry->data.data() + data_entry->data.size(), data_block->data);

    data_block->rank = data_entry->dims.size();
    log(LogLevel::INFO, "data rank is: " + std::to_string(data_block->rank));

    DIMS* dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; ++i) {
        initDimBlock(&dims[i]);

        dims[i].dim_n = data_entry->dims[i].size() / size_of_uda_type(data_entry->dim_types[i]);
        log(LogLevel::INFO, "dim " + std::to_string(i) + " length: " + std::to_string(dims[i].dim_n));

        dims[i].dim = nullptr;
        dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        dims[i].compressed = 1;
        dims[i].method = 0;
        dims[i].dim0 = 0.0;
        dims[i].diff = 1.0;
    }
    data_block->dims = dims;
    data_block->order = data_entry->order;

    log_datablock_status(data_block, "data_block from cache (data array only retreived)");
    return true;
}

bool ram_cache::uda::copy_error_high_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block)
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

    if (data_entry->error_high.empty()) {
        throw std::runtime_error("error high data does not exist for data item \"" + key + "\"");
    }

    initDataBlock(data_block);
    data_block->data_type = data_entry->data_type;
    data_block->data_n = data_entry->error_high.size() / size_of_uda_type(data_entry->error_type);

    log(LogLevel::INFO, "data size is: " + std::to_string(data_block->data_n));

    data_block->data = (char*)malloc(data_entry->error_high.size());
    std::copy(data_entry->error_high.data(), data_entry->error_high.data() + data_entry->error_high.size(),
              data_block->data);

    data_block->rank = data_entry->dims.size();
    log(LogLevel::INFO, "data rank is: " + std::to_string(data_block->rank));

    DIMS* dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; ++i) {
        initDimBlock(&dims[i]);

        dims[i].dim_n = data_entry->dims[i].size() / size_of_uda_type(data_entry->dim_types[i]);
        log(LogLevel::INFO, "dim " + std::to_string(i) + " length: " + std::to_string(dims[i].dim_n));

        dims[i].dim = nullptr;
        dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        dims[i].compressed = 1;
        dims[i].method = 0;
        dims[i].dim0 = 0.0;
        dims[i].diff = 1.0;
    }
    data_block->dims = dims;
    data_block->order = data_entry->order;

    log_datablock_status(data_block, "data_block from cache (errhi array only retreived)");
    return true;
}

bool ram_cache::uda::copy_time_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block)
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

    if (data_entry->order == -1) {
        throw std::runtime_error("no time data avaialable when requested for key \"" + key + "\"");
    }

    int order = data_entry->order;
    auto cache_dim = data_entry->dims[order];
    initDataBlock(data_block);
    data_block->data_type = data_entry->dim_types[order];
    data_block->data_n = cache_dim.size() / size_of_uda_type(data_block->data_type);

    log(LogLevel::INFO, "data size is: " + std::to_string(data_block->data_n));

    data_block->data = (char*)malloc(cache_dim.size());
    std::copy(cache_dim.data(), cache_dim.data() + cache_dim.size(), data_block->data);

    data_block->rank = 1;
    log(LogLevel::INFO, "data rank is: " + std::to_string(data_block->rank));

    DIMS* dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    initDimBlock(dims);
    dims->dim_n = data_block->data_n;
    log(LogLevel::INFO, "time dim length: " + std::to_string(dims->dim_n));

    // set arbitrary units for the dim block
    dims->dim = nullptr;
    dims->data_type = UDA_TYPE_UNSIGNED_INT;
    dims->compressed = 1;
    dims->method = 0;
    dims->dim0 = 0.0;
    dims->diff = 1.0;
    data_block->dims = dims;
    data_block->order = -1;

    log_datablock_status(data_block, "data_block from cache, after only retrieving time dim");
    return true;
}

bool ram_cache::uda::copy_dim_from_cache(const ram_cache::RamCache& cache, const std::string& key, unsigned int i, DATA_BLOCK* data_block)
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

    if (i > data_entry->dims.size()) {
        throw std::runtime_error("dimension " + std::to_string(i) + " requested for data item \"" + key +
                                 "\", which is only rank " + std::to_string(data_entry->dims.size()));
    }

    auto cache_dim = data_entry->dims[i];
    initDataBlock(data_block);
    data_block->data_type = data_entry->dim_types[i];
    data_block->data_n = cache_dim.size() / size_of_uda_type(data_block->data_type);

    log(LogLevel::INFO, "data size is: " + std::to_string(data_block->data_n));

    data_block->data = (char*)malloc(cache_dim.size());
    std::copy(cache_dim.data(), cache_dim.data() + cache_dim.size(), data_block->data);

    data_block->rank = 1;
    log(LogLevel::INFO, "data rank is: " + std::to_string(data_block->rank));

    DIMS* dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    initDimBlock(dims);
    dims->dim_n = data_block->data_n;
    log(LogLevel::INFO, "dim " + std::to_string(i) + " length: " + std::to_string(dims->dim_n));

    // set arbitrary units for the dim block
    dims->dim = nullptr;
    dims->data_type = UDA_TYPE_UNSIGNED_INT;
    dims->compressed = 1;
    dims->method = 0;
    dims->dim0 = 0.0;
    dims->diff = 1.0;
    data_block->dims = dims;
    data_block->order = -1;

    log_datablock_status(data_block, "data_block from cache after retrieving only dim " + std::to_string(i));
    return true;
}

bool ram_cache::uda::copy_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block)
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
    data_block->data_n = data_entry->data.size() / size_of_uda_type(data_entry->data_type);

    log(LogLevel::INFO, "data size is: " + std::to_string(data_block->data_n));

    data_block->data = (char*)malloc(data_entry->data.size());
    std::copy(data_entry->data.data(), data_entry->data.data() + data_entry->data.size(), data_block->data);
    if (!data_entry->error_high.empty()) {
        data_block->errhi = (char*)malloc(data_entry->error_high.size());
        std::copy(data_entry->error_high.data(), data_entry->error_high.data() + data_entry->error_high.size(),
                  data_block->errhi);
    }
    if (!data_entry->error_low.empty()) {
        data_block->errlo = (char*)malloc(data_entry->error_low.size());
        std::copy(data_entry->error_low.data(), data_entry->error_low.data() + data_entry->error_low.size(),
                  data_block->errlo);
    }
    data_block->rank = data_entry->dims.size();
    log(LogLevel::INFO, "data rank is: " + std::to_string(data_block->rank));

    DIMS* dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; ++i) {
        initDimBlock(&dims[i]);

        dims[i].data_type = data_entry->dim_types[i];
        dims[i].dim_n = data_entry->dims[i].size() / size_of_uda_type(dims[i].data_type);

        log(LogLevel::INFO, "dim " + std::to_string(i) + " length: " + std::to_string(dims[i].dim_n));

        dims[i].dim = (char*)malloc(data_entry->dims[i].size());
        std::copy(data_entry->dims[i].data(), data_entry->dims[i].data() + data_entry->dims[i].size(), dims[i].dim);
    }
    data_block->dims = dims;
    data_block->order = data_entry->order;

    log_datablock_status(data_block, "data_block from cache");
    return true;
}
