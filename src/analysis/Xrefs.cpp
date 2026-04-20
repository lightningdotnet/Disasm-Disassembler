#include "Xrefs.h"

#include "disasm/Engine.h"
#include "disasm/DecodedIndex.h"
#include "pe/PEImage.h"

namespace analysis {

namespace {

bool is_branch_cat(ZydisInstructionCategory c) {
    return c == ZYDIS_CATEGORY_CALL
        || c == ZYDIS_CATEGORY_UNCOND_BR
        || c == ZYDIS_CATEGORY_COND_BR;
}

XrefKind code_kind(ZydisInstructionCategory c) {
    if (c == ZYDIS_CATEGORY_CALL)      return XrefKind::Call;
    if (c == ZYDIS_CATEGORY_COND_BR)   return XrefKind::Cond;
    return XrefKind::Jump;
}

} // namespace

void Xrefs::clear() { m_to.clear(); }

void Xrefs::build(const disasm::Engine& engine,
                  const disasm::DecodedIndex& index,
                  const pe::PEImage& image) {
    m_to.clear();

    const uint8_t* file = image.data();
    const ZydisDecoder* dec = engine.decoder();

    ZydisDecodedInstruction instr;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

    for (const auto& e : index.entries()) {
        if (e.length < 1) continue;
        const uint8_t* p = file + e.file_off;
        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(dec, p, e.length, &instr, operands))) continue;

        // Code xrefs: first operand of branches is the target
        if (is_branch_cat(instr.meta.category)) {
            for (uint8_t i = 0; i < instr.operand_count_visible; ++i) {
                const auto& op = operands[i];
                if (op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && op.imm.is_relative) {
                    uint64_t abs = 0;
                    ZydisCalcAbsoluteAddress(&instr, &op, e.addr, &abs);
                    m_to[abs].push_back(Xref{ e.addr, code_kind(instr.meta.category) });
                    break;
                }
            }
        }

        // Data xrefs: RIP-relative memory operands
        for (uint8_t i = 0; i < instr.operand_count_visible; ++i) {
            const auto& op = operands[i];
            if (op.type != ZYDIS_OPERAND_TYPE_MEMORY) continue;
            if (op.mem.base != ZYDIS_REGISTER_RIP && op.mem.base != ZYDIS_REGISTER_EIP) continue;
            if (op.mem.disp.has_displacement == ZYAN_FALSE) continue;

            uint64_t abs = 0;
            if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&instr, &op, e.addr, &abs))) continue;

            XrefKind k = XrefKind::DataRead;
            if (instr.mnemonic == ZYDIS_MNEMONIC_LEA) k = XrefKind::DataLea;
            else if (op.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE) k = XrefKind::DataWrite;
            m_to[abs].push_back(Xref{ e.addr, k });
        }
    }
}

const std::vector<Xref>* Xrefs::to(uint64_t addr) const {
    auto it = m_to.find(addr);
    return it == m_to.end() ? nullptr : &it->second;
}

} // namespace analysis
