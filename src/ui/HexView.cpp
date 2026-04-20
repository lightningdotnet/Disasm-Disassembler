#include "HexView.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

void render_hex_view(AppContext& ctx) {
    if (!ctx.show_hex) return;
    if (!ImGui::Begin(ICON_BINARY "  Hex", &ctx.show_hex)) { ImGui::End(); return; }

    auto* doc = ctx.current();
    if (!doc) {
        ImGui::TextDisabled("\xe2\x80\x94");
        ImGui::End();
        return;
    }

    auto* sec = doc->image.find_section_by_va(doc->selected_addr);
    if (!sec || sec->raw_size == 0) {
        ImGui::TextDisabled("No bytes at selected address.");
        ImGui::End();
        return;
    }

    const uint8_t* base = doc->image.data() + sec->raw_address;
    const uint64_t va0  = doc->image.image_base() + sec->virtual_address;
    const size_t   total_rows = (sec->raw_size + 15) / 16;

    static uint64_t        last_addr = UINT64_MAX;
    static uint32_t        last_sec  = 0;
    static const Document* last_doc  = nullptr;
    const uint32_t sec_id = sec->virtual_address;
    if (last_addr != doc->selected_addr || last_sec != sec_id || last_doc != doc) {
        if (doc->selected_addr >= va0) {
            const uint64_t row = (doc->selected_addr - va0) / 16;
            if (row < total_rows) {
                const float h = ImGui::GetTextLineHeightWithSpacing();
                ImGui::SetScrollY(h * (float)row - ImGui::GetContentRegionAvail().y * 0.25f);
            }
        }
        last_addr = doc->selected_addr;
        last_sec  = sec_id;
        last_doc  = doc;
    }

    ImGuiListClipper clipper;
    clipper.Begin((int)total_rows);
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const size_t   offs = (size_t)row * 16;
            const uint64_t va   = va0 + offs;

            char line[128];
            int  n = std::snprintf(line, sizeof(line), "%016llx  ", (unsigned long long)va);
            char ascii[17] = {};
            for (int i = 0; i < 16; ++i) {
                if (offs + i < sec->raw_size) {
                    const uint8_t b = base[offs + i];
                    n += std::snprintf(line + n, sizeof(line) - n, "%02x ", b);
                    ascii[i] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
                } else {
                    n += std::snprintf(line + n, sizeof(line) - n, "   ");
                    ascii[i] = ' ';
                }
                if (i == 7) n += std::snprintf(line + n, sizeof(line) - n, " ");
            }
            std::snprintf(line + n, sizeof(line) - n, " %s", ascii);
            ImGui::TextUnformatted(line);
        }
    }

    ImGui::End();
}

} // namespace ui
