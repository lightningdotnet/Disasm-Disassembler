#include "StatusBar.h"

#include "AppContext.h"
#include "Icons.h"

#include "imgui.h"

namespace ui {

// Renders inline (no Begin) — caller is responsible for window/host context.
void render_status_bar(AppContext& ctx) {
    if (auto* doc = ctx.current()) {
        const char* arch = doc->image.is_64bit() ? "x64" : "x86";
        ImGui::Text(ICON_DOC "  %s    %s    %zu insns    %zu syms    %zu strings    %s",
                    doc->display_name.c_str(),
                    arch,
                    doc->index.size(),
                    doc->symbols.size(),
                    doc->strings.size(),
                    doc->status_message.empty() ? "ready" : doc->status_message.c_str());
    } else {
        ImGui::TextDisabled(
            ICON_OPEN "  No file loaded \xe2\x80\x94 drop a PE here, or File \xe2\x86\x92 Open\xe2\x80\xa6");
    }
}

} // namespace ui
