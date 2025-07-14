#pragma once

#include <boost/range/numeric.hpp>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

// UDA includes
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <server/getServerEnvironment.h>
#include <utils/print_uda_structs.hpp>

#include "utils/ram_cache.hpp"

namespace ram_cache::uda
{

class UDACacheEntry : public CacheEntry
{
  public:
    UDACacheEntry() = default;
    [[nodiscard]] size_t size() const override
    {
        int dim_size = boost::accumulate(dims, 0, [](int sum, auto& item) { return sum + item.size(); });
        return data.size() + error_high.size() + error_low.size() + dim_size;
    }

    std::vector<char> data;
    std::vector<char> error_high;
    std::vector<char> error_low;
    std::vector<std::vector<char>> dims;
    int order = -1;
    int data_type = UDA_TYPE_UNKNOWN;
    std::vector<int> dim_types;
    int error_type = UDA_TYPE_UNKNOWN;
};

bool copy_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block);
bool copy_data_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block);
bool copy_error_high_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block);
bool copy_time_from_cache(const ram_cache::RamCache& cache, const std::string& key, DATA_BLOCK* data_block);
bool copy_dim_from_cache(const ram_cache::RamCache& cache, const std::string& key, unsigned int i,
                         DATA_BLOCK* data_block);

} // namespace ram_cache::uda
