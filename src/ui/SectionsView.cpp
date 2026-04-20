#include "SectionsView.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

void render_sections_view(AppContext& ctx) {
    if (!ctx.show_sections) return;
    if (!ImGui::Begin(ICON_GRID "  Sections", &ctx.show_sections)) {
        ImGui::End();
        return;
    }

    auto* doc = ctx.current();
    if (!doc) {
        ImGui::TextDisabled("\xe2\x80\x94");
        ImGui::End();
        return;
    }

    const ImGuiTableFlags flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("sections", 6, flags)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("RVA");
        ImGui::TableSetupColumn("VSize");
        ImGui::TableSetupColumn("RawOff");
        ImGui::TableSetupColumn("RawSize");
        ImGui::TableSetupColumn("Flags");
        ImGui::TableHeadersRow();

        for (const auto& s : doc->image.sections()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            char label[64];
            std::snprintf(label, sizeof(label), "%s##%08x", s.name.c_str(), s.virtual_address);
            if (ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns)) {
                uint64_t va = doc->image.image_base() + s.virtual_address;
                doc->pending_scroll_va = va;
                doc->nav.go(va);
                doc->selected_addr = va;
            }
            ImGui::TableNextColumn(); ImGui::Text("%08x", s.virtual_address);
            ImGui::TableNextColumn(); ImGui::Text("%08x", s.virtual_size);
            ImGui::TableNextColumn(); ImGui::Text("%08x", s.raw_address);
            ImGui::TableNextColumn(); ImGui::Text("%08x", s.raw_size);
            ImGui::TableNextColumn();
            char flg[4] = "---";
            if (s.executable()) flg[0] = 'X';
            if (s.readable())   flg[1] = 'R';
            if (s.writable())   flg[2] = 'W';
            ImGui::TextUnformatted(flg);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

} // namespace ui
