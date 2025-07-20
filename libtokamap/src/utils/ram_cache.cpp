#include "ram_cache.hpp"

#include <string>

bool libtokamap::RamCache::contains(const std::string& key) const { return m_entries.contains(key); }
