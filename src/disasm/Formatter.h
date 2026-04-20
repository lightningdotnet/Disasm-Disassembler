#pragma once

#include <Zydis/Zydis.h>
#include <string>

namespace analysis { class SymbolStore; }

namespace disasm {

class Formatter {
public:
    bool init(const analysis::SymbolStore* symbols, bool is_64bit);

    // Format one instruction. runtime_address is the VA of the instruction.
    std::string format(const ZydisDecodedInstruction& instr,
                       const ZydisDecodedOperand*     operands,
                       uint64_t runtime_address) const;

private:
    static ZyanStatus print_address_abs_hook(
        const ZydisFormatter*, ZydisFormatterBuffer*, ZydisFormatterContext*);
    static ZyanStatus print_imm_hook(
        const ZydisFormatter*, ZydisFormatterBuffer*, ZydisFormatterContext*);

    ZydisFormatter                m_formatter{};
    ZydisFormatterFunc            m_default_addr_abs = nullptr;
    ZydisFormatterFunc            m_default_imm      = nullptr;
    const analysis::SymbolStore*  m_symbols          = nullptr;
    bool                          m_is64             = false;
};

} // namespace disasm
