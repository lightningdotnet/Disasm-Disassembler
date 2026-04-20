#include "Strings.h"

#include "pe/PEImage.h"

#include <cstdint>

namespace analysis {

namespace {

bool is_printable_ascii(uint8_t c) {
    return (c >= 0x20 && c < 0x7F) || c == '\t' || c == '\n' || c == '\r';
}

bool is_printable_wc(uint16_t c) {
    // Basic-plane printable; crude filter
    if (c < 0x20) return c == '\t' || c == '\n' || c == '\r';
    if (c == 0x7F) return false;
    if (c >= 0xD800 && c <= 0xDFFF) return false; // surrogates
    return c < 0xFFFE;
}

std::string utf16_to_utf8(const uint16_t* p, size_t n) {
    std::string out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        uint32_t cp = p[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < n) {
            uint32_t low = p[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                ++i;
            }
        }
        if (cp < 0x80) out.push_back((char)cp);
        else if (cp < 0x800) {
            out.push_back((char)(0xC0 | (cp >> 6)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back((char)(0xE0 |  (cp >> 12)));
            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back((char)(0x80 |  (cp & 0x3F)));
        } else {
            out.push_back((char)(0xF0 |  (cp >> 18)));
            out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back((char)(0x80 | ((cp >>  6) & 0x3F)));
            out.push_back((char)(0x80 |  (cp & 0x3F)));
        }
    }
    return out;
}

} // namespace

void Strings::clear() { m_strings.clear(); m_strings.shrink_to_fit(); }

void Strings::scan(const pe::PEImage& image, size_t min_len) {
    m_strings.clear();
    const uint8_t* file = image.data();
    const uint64_t base = image.image_base();

    for (const auto& sec : image.sections()) {
        if (!sec.readable()) continue;
        if (sec.executable()) continue;   // skip .text
        if (sec.raw_size == 0) continue;

        const uint8_t* p   = file + sec.raw_address;
        const uint8_t* end = p + sec.raw_size;
        const uint64_t va0 = base + sec.virtual_address;

        // ASCII runs
        const uint8_t* q = p;
        while (q < end) {
            const uint8_t* start = q;
            while (q < end && is_printable_ascii(*q)) ++q;
            const size_t len = static_cast<size_t>(q - start);
            if (len >= min_len) {
                StringRef s;
                s.va     = va0 + (uint32_t)(start - p);
                s.length = (uint32_t)len;
                s.wide   = false;
                s.text.assign(reinterpret_cast<const char*>(start), len);
                m_strings.push_back(std::move(s));
            }
            if (q < end) ++q;
        }

        // UTF-16LE runs (crude — scans unaligned)
        const uint8_t* r = p;
        while (r + 1 < end) {
            const uint8_t* start = r;
            size_t n = 0;
            while (r + 1 < end) {
                uint16_t wc = (uint16_t)r[0] | ((uint16_t)r[1] << 8);
                if (wc == 0 || !is_printable_wc(wc)) break;
                r += 2;
                ++n;
            }
            if (n >= min_len) {
                StringRef s;
                s.va     = va0 + (uint32_t)(start - p);
                s.length = (uint32_t)n;
                s.wide   = true;
                s.text   = utf16_to_utf8(reinterpret_cast<const uint16_t*>(start), n);
                m_strings.push_back(std::move(s));
            }
            if (r + 1 < end) r += 2;
        }
    }
}

} // namespace analysis
