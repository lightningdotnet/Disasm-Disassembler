#include "ImportsView.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

void render_imports_view(AppContext& ctx) {
    if (!ctx.show_imports) return;
    if (!ImGui::Begin(ICON_LINK "  Imports", &ctx.show_imports)) {
        ImGui::End();
        return;
    }

    auto* doc = ctx.current();
    if (!doc) {
        ImGui::TextDisabled("\xe2\x80\x94");
        ImGui::End();
        return;
    }

    const auto& mods = doc->image.imports();
    for (size_t i = 0; i < mods.size(); ++i) {
        const auto& mod = mods[i];
        ImGui::PushID((int)i);
        char hdr[128];
        std::snprintf(hdr, sizeof(hdr), "%s  (%zu)", mod.dll.c_str(), mod.functions.size());
        if (ImGui::TreeNode(hdr)) {
            for (const auto& f : mod.functions) {
                char row[192];
                if (!f.name.empty()) {
                    std::snprintf(row, sizeof(row), "%016llx  %s",
                                  (unsigned long long)f.iat_va, f.name.c_str());
                } else {
                    std::snprintf(row, sizeof(row), "%016llx  ordinal #%u",
                                  (unsigned long long)f.iat_va, f.ordinal);
                }
                if (ImGui::Selectable(row)) {
                    doc->pending_scroll_va = f.iat_va;
                    doc->nav.go(f.iat_va);
                    doc->selected_addr = f.iat_va;
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::End();
}

} // namespace ui
