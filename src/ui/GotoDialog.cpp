#include "GotoDialog.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

#include <cstdio>

namespace ui {

void render_goto_dialog(AppContext& ctx) {
    if (!ctx.goto_open) return;

    auto* doc = ctx.current();
    if (!doc) { ctx.goto_open = false; return; }

    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_Always);
    if (ImGui::Begin(ICON_SEARCH "  Go to address", &ctx.goto_open,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        static char buf[24] = {};
        ImGui::SetKeyboardFocusHere();
        bool submit = ImGui::InputTextWithHint("##goto", "hex VA, e.g. 140001000",
            buf, sizeof(buf),
            ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("Go") || submit) {
            unsigned long long va = 0;
            std::sscanf(buf, "%llx", &va);
            if (va != 0) {
                doc->pending_scroll_va = va;
                doc->nav.go(va);
                doc->selected_addr = va;
            }
            ctx.goto_open = false;
            buf[0] = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ctx.goto_open = false;
            buf[0] = 0;
        }
    }
    ImGui::End();
}

} // namespace ui
