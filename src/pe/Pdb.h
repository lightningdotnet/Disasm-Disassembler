#pragma once

#include <cstdint>

namespace analysis { class SymbolStore; }

namespace pe {

class PEImage;

// Pragmatic PDB loader via dbghelp.dll (always present on Windows).
// Populates the SymbolStore with all public/function symbols reachable.
class PdbLoader {
public:
    PdbLoader() = default;
    ~PdbLoader();

    PdbLoader(const PdbLoader&)            = delete;
    PdbLoader& operator=(const PdbLoader&) = delete;

    bool load(const PEImage& image, analysis::SymbolStore& symbols);
    void unload();

    bool loaded() const { return m_module_base != 0; }

private:
    uint64_t m_module_base = 0;
};

} // namespace pe
