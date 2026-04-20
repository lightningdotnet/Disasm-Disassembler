#include "ExportsView.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

void render_exports_view(AppContext& ctx) {
    if (!ctx.show_exports) return;
    if (!ImGui::Begin(ICON_TAG "  Exports", &ctx.show_exports)) {
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
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Sortable;

    if (ImGui::BeginTable("exports", 4, flags)) {
        ImGui::TableSetupColumn("Ordinal");
        ImGui::TableSetupColumn("RVA");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Forwarder");
        ImGui::TableHeadersRow();

        for (const auto& e : doc->image.exports()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            char label[32];
            std::snprintf(label, sizeof(label), "#%u##%u", e.ordinal, e.ordinal);
            if (ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns)) {
                uint64_t va = doc->image.image_base() + e.rva;
                doc->pending_scroll_va = va;
                doc->nav.go(va);
                doc->selected_addr = va;
            }
            ImGui::TableNextColumn(); ImGui::Text("%08x", e.rva);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(e.name.empty() ? "(by ordinal)" : e.name.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(e.forwarder.c_str());
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

} // namespace ui
