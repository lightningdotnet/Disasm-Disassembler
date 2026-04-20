#pragma once

#include <cstdint>
#include <vector>

namespace pe     { class PEImage; }
namespace disasm { class Engine; }

namespace disasm {

struct IndexEntry {
    uint64_t addr;      // VA
    uint32_t file_off;  // precomputed file offset
    uint16_t length;    // instruction byte length
    uint16_t _pad;
};
static_assert(sizeof(IndexEntry) == 16, "IndexEntry should stay 16 bytes");

class DecodedIndex {
public:
    // Linear sweep through all executable sections.
    void build(const Engine& engine, const pe::PEImage& image);
    void clear();

    const std::vector<IndexEntry>& entries() const { return m_entries; }
    size_t size() const { return m_entries.size(); }

    // Binary search; returns index or SIZE_MAX.
    size_t find_by_va(uint64_t va) const;
    // First entry at or after va.
    size_t lower_bound_va(uint64_t va) const;

private:
    std::vector<IndexEntry> m_entries;
};

} // namespace disasm
