#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

namespace analysis {

class SymbolStore {
public:
    // Higher priority overwrites lower.
    enum class Source : uint8_t {
        Auto      = 0,  // sub_...
        ImportIat = 1,
        Export    = 2,
        Pdb       = 3,
        User      = 4,
    };

    void clear();

    // Insert; ignored if existing entry is from a higher-priority source.
    // Returns a stable const char* that lives as long as the store.
    const char* add(uint64_t addr, std::string name, Source src);

    // Create sub_XXXXXXXX entry if addr has no symbol yet.
    const char* ensure_sub(uint64_t addr, bool is_64bit);

    // Lookup; nullptr if none.
    const char* find(uint64_t addr) const;

    size_t size() const { return m_map.size(); }

private:
    struct Entry {
        const char* name;
        Source      src;
    };
    std::unordered_map<uint64_t, Entry> m_map;
    std::deque<std::string>             m_storage;   // stable pointers
};

} // namespace analysis
