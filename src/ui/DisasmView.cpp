#include "DisasmView.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

void render_disasm_view(AppContext& ctx) {
    if (!ctx.show_disasm) return;
    if (!ImGui::Begin(ICON_BINARY "  Disassembly", &ctx.show_disasm)) {
        ImGui::End();
        return;
    }

    auto* doc = ctx.current();
    if (!doc) {
        ImGui::TextDisabled(ICON_OPEN "  No file loaded.");
        ImGui::End();
        return;
    }

    const auto& entries = doc->index.entries();
    if (entries.empty()) {
        ImGui::TextDisabled("No executable bytes.");
        ImGui::End();
        return;
    }

    if (doc->pending_scroll_va != UINT64_MAX) {
        size_t row = doc->index.lower_bound_va(doc->pending_scroll_va);
        if (row >= entries.size()) row = entries.size() - 1;
        doc->selected_addr = entries[row].addr;
        const float h = ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetScrollY(h * (float)row - ImGui::GetContentRegionAvail().y * 0.25f);
        doc->pending_scroll_va = UINT64_MAX;
    }

    const ZydisDecoder* dec  = doc->engine.decoder();
    const uint8_t*      file = doc->image.data();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGuiListClipper clipper;
    clipper.Begin((int)entries.size());
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const auto& e = entries[row];

            if (const char* sym = doc->symbols.find(e.addr)) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.78f, 1.00f, 1.0f));
                ImGui::Text("%s:", sym);
                ImGui::PopStyleColor();
            }

            ZydisDecodedInstruction instr;
            ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
            const uint8_t* p = file + e.file_off;

            char bytes_buf[48] = {};
            int bpos = 0;
            const int show_bytes = (e.length > 10) ? 10 : e.length;
            for (int i = 0; i < show_bytes && bpos + 3 < (int)sizeof(bytes_buf); ++i) {
                bpos += std::snprintf(bytes_buf + bpos, sizeof(bytes_buf) - bpos, "%02X ", p[i]);
            }
            if (e.length > show_bytes) {
                std::snprintf(bytes_buf + bpos, sizeof(bytes_buf) - bpos, "..");
            }

            std::string asm_text;
            if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(dec, p, e.length, &instr, operands))) {
                asm_text = doc->formatter.format(instr, operands, e.addr);
            } else {
                char db[16];
                std::snprintf(db, sizeof(db), "db 0x%02X", p[0]);
                asm_text = db;
            }

            char row_label[384];
            std::snprintf(row_label, sizeof(row_label), "%016llx  %-32s  %s##%llx",
                          (unsigned long long)e.addr, bytes_buf,
                          asm_text.c_str(), (unsigned long long)e.addr);

            const bool selected = (doc->selected_addr == e.addr);
            if (ImGui::Selectable(row_label, selected,
                                  ImGuiSelectableFlags_AllowDoubleClick |
                                  ImGuiSelectableFlags_SpanAllColumns)) {
                doc->selected_addr = e.addr;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    for (uint8_t i = 0; i < instr.operand_count_visible; ++i) {
                        const auto& op = operands[i];
                        if (op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && op.imm.is_relative) {
                            uint64_t abs = 0;
                            ZydisCalcAbsoluteAddress(&instr, &op, e.addr, &abs);
                            doc->pending_scroll_va = abs;
                            doc->nav.go(abs);
                            break;
                        }
                    }
                }
            }
        }
    }

    ImGui::PopStyleVar();
    ImGui::End();
}

} // namespace ui
