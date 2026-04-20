#include "XrefsPopup.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

static const char* kind_string(analysis::XrefKind k) {
    using K = analysis::XrefKind;
    switch (k) {
        case K::Call:      return "call";
        case K::Jump:      return "jmp";
        case K::Cond:      return "jcc";
        case K::DataLea:   return "lea";
        case K::DataRead:  return "read";
        case K::DataWrite: return "write";
    }
    return "?";
}

void render_xrefs_popup(AppContext& ctx) {
    if (!ctx.xrefs_open) return;

    auto* doc = ctx.current();
    if (!doc) { ctx.xrefs_open = false; return; }

    ImGui::SetNextWindowSize(ImVec2(640, 400), ImGuiCond_FirstUseEver);
    char title[80];
    std::snprintf(title, sizeof(title), ICON_LINK "  Xrefs to %016llx",
                  (unsigned long long)ctx.xrefs_target);
    if (ImGui::Begin(title, &ctx.xrefs_open)) {
        const auto* refs = doc->xrefs.to(ctx.xrefs_target);
        if (!refs || refs->empty()) {
            ImGui::TextDisabled("No cross-references.");
        } else {
            if (ImGui::BeginTable("xrefs", 2,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Kind");
                ImGui::TableSetupColumn("From");
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < refs->size(); ++i) {
                    const auto& x = (*refs)[i];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(kind_string(x.kind));
                    ImGui::TableNextColumn();
                    char lab[48];
                    std::snprintf(lab, sizeof(lab), "%016llx##%zu",
                                  (unsigned long long)x.from, i);
                    if (ImGui::Selectable(lab)) {
                        doc->pending_scroll_va = x.from;
                        doc->nav.go(x.from);
                        doc->selected_addr = x.from;
                        ctx.xrefs_open = false;
                    }
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

} // namespace ui
