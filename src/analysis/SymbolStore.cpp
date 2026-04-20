#include "SymbolStore.h"

#include <cstdio>

namespace analysis {

void SymbolStore::clear() {
    m_map.clear();
    m_storage.clear();
}

const char* SymbolStore::add(uint64_t addr, std::string name, Source src) {
    auto it = m_map.find(addr);
    if (it != m_map.end() && static_cast<uint8_t>(it->second.src) >= static_cast<uint8_t>(src)) {
        return it->second.name;
    }
    m_storage.push_back(std::move(name));
    const char* p = m_storage.back().c_str();
    m_map[addr] = Entry{ p, src };
    return p;
}

const char* SymbolStore::ensure_sub(uint64_t addr, bool is_64bit) {
    auto it = m_map.find(addr);
    if (it != m_map.end()) return it->second.name;

    char buf[32];
    if (is_64bit) std::snprintf(buf, sizeof(buf), "sub_%016llX", (unsigned long long)addr);
    else          std::snprintf(buf, sizeof(buf), "sub_%08X",   (unsigned)addr);

    return add(addr, buf, Source::Auto);
}

const char* SymbolStore::find(uint64_t addr) const {
    auto it = m_map.find(addr);
    return it == m_map.end() ? nullptr : it->second.name;
}

} // namespace analysis
