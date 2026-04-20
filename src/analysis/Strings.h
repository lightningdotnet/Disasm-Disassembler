#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pe { class PEImage; }

namespace analysis {

struct StringRef {
    uint64_t    va       = 0;
    uint32_t    length   = 0;   // character count (not bytes)
    bool        wide     = false; // true = UTF-16LE, false = ASCII
    std::string text;            // UTF-8
};

class Strings {
public:
    void scan(const pe::PEImage& image, size_t min_len = 4);
    void clear();

    const std::vector<StringRef>& all() const { return m_strings; }
    size_t size() const { return m_strings.size(); }

private:
    std::vector<StringRef> m_strings;
};

} // namespace analysis
