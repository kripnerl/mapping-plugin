#pragma once

#include <boost/range/numeric.hpp>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

// LibTokaMap includes
#include <utils/ram_cache.hpp>

// UDA includes
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <server/getServerEnvironment.h>

namespace json_plugin
{

class UDACacheEntry : public libtokamap::CacheEntry
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

void copy_to_cache(libtokamap::RamCache& ram_cache, const std::string& key, const DATA_BLOCK* data_block);
bool copy_from_cache(const libtokamap::RamCache& cache, const std::string& key, DATA_BLOCK* data_block);

} // namespace json_plugin
