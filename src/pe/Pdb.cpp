#include "Pdb.h"

#include "PEImage.h"
#include "analysis/SymbolStore.h"

#include <windows.h>
#include <dbghelp.h>
#include <mutex>
#include <string>

namespace pe {

namespace {

// dbghelp is process-global and not thread-safe; serialize all calls.
std::mutex& dbghelp_mutex() {
    static std::mutex m;
    return m;
}

struct EnumCtx {
    analysis::SymbolStore* symbols;
    uint64_t image_base;
};

BOOL CALLBACK enum_cb(PSYMBOL_INFO info, ULONG /*size*/, PVOID user) {
    auto* ctx = static_cast<EnumCtx*>(user);
    if (!info || !ctx || !info->Address) return TRUE;
    // SymFromAddr names include demangled C++ when SYMOPT_UNDNAME is set.
    std::string name(info->Name, info->NameLen);
    ctx->symbols->add(info->Address, std::move(name),
                      analysis::SymbolStore::Source::Pdb);
    return TRUE;
}

} // namespace

PdbLoader::~PdbLoader() { unload(); }

bool PdbLoader::load(const PEImage& image, analysis::SymbolStore& symbols) {
    if (image.image_size() == 0) return false;

    std::lock_guard<std::mutex> lk(dbghelp_mutex());

    HANDLE proc = ::GetCurrentProcess();
    ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES |
                    SYMOPT_OMAP_FIND_NEAREST);

    if (!::SymInitializeW(proc, nullptr, FALSE)) {
        // Might already be initialised by another loader — not fatal.
    }

    const DWORD64 base = static_cast<DWORD64>(image.image_base());
    const DWORD    sz  = static_cast<DWORD>(image.image_size());

    DWORD64 mod = ::SymLoadModuleExW(
        proc, nullptr, image.path().c_str(), nullptr,
        base, sz, nullptr, 0);
    if (!mod) return false;

    m_module_base = mod;

    EnumCtx ctx{ &symbols, image.image_base() };
    ::SymEnumSymbols(proc, mod, "*", enum_cb, &ctx);
    return true;
}

void PdbLoader::unload() {
    if (m_module_base != 0) {
        std::lock_guard<std::mutex> lk(dbghelp_mutex());
        ::SymUnloadModule64(::GetCurrentProcess(), m_module_base);
        m_module_base = 0;
    }
}

} // namespace pe
