#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace pe     { class PEImage; }
namespace disasm { class Engine; class DecodedIndex; }

namespace analysis {

enum class XrefKind : uint8_t {
    Call,
    Jump,
    Cond,
    DataLea,
    DataRead,
    DataWrite,
};

struct Xref {
    uint64_t from;
    XrefKind kind;
};

class Xrefs {
public:
    void build(const disasm::Engine& engine,
               const disasm::DecodedIndex& index,
               const pe::PEImage& image);
    void clear();

    // Returns pointer to vector of xrefs targeting `to`, or nullptr.
    const std::vector<Xref>* to(uint64_t addr) const;

    // Every target we know of (for the "Callers of" table, etc.)
    const std::unordered_map<uint64_t, std::vector<Xref>>& map() const { return m_to; }

private:
    std::unordered_map<uint64_t, std::vector<Xref>> m_to;
};

} // namespace analysis
