#pragma once

#include <boost/range/algorithm/find.hpp>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <string>
#include <unordered_map>
#include <optional>
#include <utility>

/*
 *
 * NOTES:
 *
 * using char instead of byte to avoid extra casting from/to datablock
 *
 */

namespace ram_cache
{

class CacheEntry {
public:
    CacheEntry() = default;
    virtual ~CacheEntry() = default;
    CacheEntry(CacheEntry&& other) = default;
    CacheEntry(const CacheEntry& other) = delete;
    CacheEntry& operator=(CacheEntry&& other) = default;
    CacheEntry& operator=(const CacheEntry& other) = delete;

    [[nodiscard]] virtual size_t size() const = 0;
};

constexpr int default_size = 100;

class RamCache
{
  public:
    RamCache()
    {
        _entries.reserve(_max_size);
    }

    explicit RamCache(uint32_t max_size) : _max_size{max_size}
    {
        _entries.reserve(_max_size);
    }

    void add(std::string key, std::unique_ptr<CacheEntry> entry)
    {
        if (_entries.size() == _max_size) {
            drop_entries();
        }
        _entries.emplace(key, std::move(entry));
    }

    [[nodiscard]] bool contains(const std::string& key) const {
        return _entries.count(key) != 0;
    }

    [[nodiscard]] std::optional<CacheEntry*> get(const std::string& key) const {
        if (!contains(key)) {
            return {};
        }
        return _entries.at(key).get();
    }

  private:
    const uint32_t _max_size = default_size;
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> _entries;

    void drop_entries() {

    }
};

} // namespace ram_cache
