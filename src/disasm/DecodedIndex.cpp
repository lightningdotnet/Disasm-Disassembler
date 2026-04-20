#include "DecodedIndex.h"

#include "Engine.h"
#include "pe/PEImage.h"

#include <algorithm>

namespace disasm {

void DecodedIndex::clear() {
    m_entries.clear();
    m_entries.shrink_to_fit();
}

void DecodedIndex::build(const Engine& engine, const pe::PEImage& image) {
    m_entries.clear();
    m_entries.reserve(1 << 20); // 1M slots to start; grows as needed

    const uint8_t* file_data = image.data();
    const uint64_t base      = image.image_base();

    for (const auto& sec : image.sections()) {
        if (!sec.executable()) continue;
        if (sec.raw_size == 0)  continue;

        const uint32_t start_off  = sec.raw_address;
        const uint32_t end_off    = sec.raw_address + sec.raw_size;
        const uint8_t* p          = file_data + start_off;
        const uint8_t* end        = file_data + end_off;
        const uint64_t va_start   = base + sec.virtual_address;

        const ZydisDecoder* dec = engine.decoder();

        size_t i = 0;
        while (p + 1 <= end) {
            ZydisDecodedInstruction instr;
            ZyanStatus s = ZydisDecoderDecodeInstruction(
                dec, nullptr, p, (ZyanUSize)(end - p), &instr);
            IndexEntry e{};
            e.addr     = va_start + i;
            e.file_off = start_off + (uint32_t)i;
            if (ZYAN_SUCCESS(s)) {
                e.length = instr.length;
            } else {
                e.length = 1; // db
            }
            m_entries.push_back(e);
            p += e.length;
            i += e.length;
        }
    }

    m_entries.shrink_to_fit();
}

size_t DecodedIndex::find_by_va(uint64_t va) const {
    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), va,
        [](const IndexEntry& e, uint64_t v) { return e.addr < v; });
    if (it == m_entries.end() || it->addr != va) return SIZE_MAX;
    return static_cast<size_t>(it - m_entries.begin());
}

size_t DecodedIndex::lower_bound_va(uint64_t va) const {
    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), va,
        [](const IndexEntry& e, uint64_t v) { return e.addr < v; });
    return static_cast<size_t>(it - m_entries.begin());
}

} // namespace disasm
