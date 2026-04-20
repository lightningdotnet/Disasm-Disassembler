#include "StringsView.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

void render_strings_view(AppContext& ctx) {
    if (!ctx.show_strings) return;
    if (!ImGui::Begin(ICON_SEARCH "  Strings", &ctx.show_strings)) {
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

    static char filter_buf[64] = {};
    ImGui::InputTextWithHint("##filter", ICON_SEARCH "  filter\xe2\x80\xa6",
                             filter_buf, sizeof(filter_buf));

    if (ImGui::BeginTable("strings", 4, flags)) {
        ImGui::TableSetupColumn("Address");
        ImGui::TableSetupColumn("Len");
        ImGui::TableSetupColumn("Enc");
        ImGui::TableSetupColumn("Text");
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        const auto& all = doc->strings.all();

        static std::vector<const analysis::StringRef*> filtered;
        filtered.clear();
        for (const auto& s : all) {
            if (filter_buf[0] == 0 || s.text.find(filter_buf) != std::string::npos)
                filtered.push_back(&s);
        }

        clipper.Begin((int)filtered.size());
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const auto* s = filtered[row];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                char label[32];
                std::snprintf(label, sizeof(label), "%016llx##%d", (unsigned long long)s->va, row);
                if (ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns)) {
                    doc->pending_scroll_va = s->va;
                    doc->nav.go(s->va);
                    doc->selected_addr = s->va;
                }
                ImGui::TableNextColumn(); ImGui::Text("%u", s->length);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(s->wide ? "W" : "A");
                ImGui::TableNextColumn(); ImGui::TextUnformatted(s->text.c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

} // namespace ui
