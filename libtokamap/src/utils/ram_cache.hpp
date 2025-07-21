#pragma once

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

namespace libtokamap
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
        m_entries.reserve(m_max_size);
    }

    explicit RamCache(uint32_t max_size) : m_max_size{max_size}
    {
        m_entries.reserve(m_max_size);
    }

    void add(std::string key, std::unique_ptr<CacheEntry> entry)
    {
        if (m_entries.size() == m_max_size) {
            drop_entries();
        }
        m_entries.emplace(key, std::move(entry));
    }

    [[nodiscard]] bool contains(const std::string& key) const;

    [[nodiscard]] std::optional<CacheEntry*> get(const std::string& key) const {
        if (!contains(key)) {
            return {};
        }
        return m_entries.at(key).get();
    }

  private:
    const uint32_t m_max_size = default_size;
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> m_entries;

    void drop_entries() {

    }
};

} // namespace ram_cache
