#include "Formatter.h"

#include "analysis/SymbolStore.h"

#include <Zycore/Format.h>

namespace disasm {

bool Formatter::init(const analysis::SymbolStore* symbols, bool is_64bit) {
    m_symbols = symbols;
    m_is64    = is_64bit;

    if (!ZYAN_SUCCESS(ZydisFormatterInit(&m_formatter, ZYDIS_FORMATTER_STYLE_INTEL))) {
        return false;
    }

    // Compact defaults
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT,  ZYAN_FALSE);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_FORCE_RELATIVE_BRANCHES, ZYAN_FALSE);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_ADDR_PADDING_ABSOLUTE,
                              ZYDIS_PADDING_DISABLED);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_ADDR_PADDING_RELATIVE,
                              ZYDIS_PADDING_DISABLED);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_DISP_PADDING,
                              ZYDIS_PADDING_DISABLED);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_IMM_PADDING,
                              ZYDIS_PADDING_DISABLED);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_PREFIXES,   ZYAN_FALSE);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_MNEMONIC,   ZYAN_FALSE);
    ZydisFormatterSetProperty(&m_formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_REGISTERS,  ZYAN_FALSE);

    // Install hooks
    m_default_addr_abs = reinterpret_cast<ZydisFormatterFunc>(&print_address_abs_hook);
    ZydisFormatterSetHook(&m_formatter,
        ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS,
        reinterpret_cast<const void**>(&m_default_addr_abs));

    m_default_imm = reinterpret_cast<ZydisFormatterFunc>(&print_imm_hook);
    ZydisFormatterSetHook(&m_formatter,
        ZYDIS_FORMATTER_FUNC_PRINT_IMM,
        reinterpret_cast<const void**>(&m_default_imm));

    return true;
}

std::string Formatter::format(const ZydisDecodedInstruction& instr,
                              const ZydisDecodedOperand*     operands,
                              uint64_t runtime_address) const {
    char buf[256];
    // Pass `this` as user_data so hooks can reach the SymbolStore.
    ZydisFormatterFormatInstruction(
        &m_formatter, &instr, operands,
        instr.operand_count_visible, buf, sizeof(buf), runtime_address,
        const_cast<Formatter*>(this));
    return std::string(buf);
}

// -----------------------------------------------------------------------------
// Hook for resolved absolute addresses: e.g. `lea rax, [rip+X]` / `call 0x140...`
ZyanStatus Formatter::print_address_abs_hook(const ZydisFormatter* f,
                                             ZydisFormatterBuffer* buffer,
                                             ZydisFormatterContext* ctx) {
    Formatter* self = static_cast<Formatter*>(ctx->user_data);
    uint64_t abs = 0;
    ZydisCalcAbsoluteAddress(ctx->instruction, ctx->operand,
                             ctx->runtime_address, &abs);
    if (self && self->m_symbols) {
        if (const char* name = self->m_symbols->find(abs)) {
            ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL);
            ZyanString* str;
            ZydisFormatterBufferGetString(buffer, &str);
            return ZyanStringAppendFormat(str, "%s", name);
        }
    }
    return self->m_default_addr_abs(f, buffer, ctx);
}

// Hook for immediates — used for relative branches that resolve to an address
ZyanStatus Formatter::print_imm_hook(const ZydisFormatter* f,
                                     ZydisFormatterBuffer* buffer,
                                     ZydisFormatterContext* ctx) {
    Formatter* self = static_cast<Formatter*>(ctx->user_data);
    if (self && self->m_symbols && ctx->operand->imm.is_relative) {
        uint64_t abs = 0;
        ZydisCalcAbsoluteAddress(ctx->instruction, ctx->operand,
                                 ctx->runtime_address, &abs);
        if (const char* name = self->m_symbols->find(abs)) {
            ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL);
            ZyanString* str;
            ZydisFormatterBufferGetString(buffer, &str);
            return ZyanStringAppendFormat(str, "%s", name);
        }
    }
    return self->m_default_imm(f, buffer, ctx);
}

} // namespace disasm
