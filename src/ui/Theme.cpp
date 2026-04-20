#include "Theme.h"

#include "platform/Win32DX11.h"

#include "imgui.h"

namespace ui {

void load_mono_font() {
    ImGuiIO& io = ImGui::GetIO();

    const float scale     = platform::current_dpi_scale();
    const float main_size = 15.0f * scale;
    const float icon_size = 14.0f * scale;

    ImFont* mono = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\consola.ttf", main_size, nullptr,
        io.Fonts->GetGlyphRangesDefault());
    if (!mono) {
        io.Fonts->AddFontDefault();
        return;
    }

    // Merge in Segoe MDL2 Assets PUA glyphs as an icon font.
    ImFontConfig cfg{};
    cfg.MergeMode        = true;
    cfg.PixelSnapH       = true;
    cfg.GlyphMinAdvanceX = main_size;       // square advance, looks aligned
    cfg.GlyphOffset.y    = 1.0f;            // nudge into baseline
    static const ImWchar icon_ranges[] = { 0xE000, 0xF8FF, 0 };
    io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\SegMDL2.ttf", icon_size, &cfg, icon_ranges);
}

void apply_theme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding    = 6.0f;
    s.ChildRounding     = 4.0f;
    s.FrameRounding     = 4.0f;
    s.PopupRounding     = 4.0f;
    s.ScrollbarRounding = 8.0f;
    s.GrabRounding      = 4.0f;
    s.TabRounding       = 4.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.PopupBorderSize   = 1.0f;
    s.WindowPadding     = ImVec2(8, 6);
    s.FramePadding      = ImVec2(8, 4);
    s.ItemSpacing       = ImVec2(8, 4);
    s.CellPadding       = ImVec2(6, 3);
    s.IndentSpacing     = 16.0f;
    s.ScrollbarSize     = 12.0f;
    s.GrabMinSize       = 8.0f;
    s.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
    s.SeparatorTextBorderSize = 1.0f;

    s.ScaleAllSizes(platform::current_dpi_scale());

    ImVec4* c = s.Colors;

    // Backgrounds (slightly warm dark)
    c[ImGuiCol_WindowBg]            = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ChildBg]             = ImVec4(0.10f, 0.10f, 0.12f, 0.00f);
    c[ImGuiCol_PopupBg]             = ImVec4(0.13f, 0.13f, 0.16f, 0.98f);
    c[ImGuiCol_MenuBarBg]           = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    c[ImGuiCol_DockingEmptyBg]      = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);

    // Text
    c[ImGuiCol_Text]                = ImVec4(0.86f, 0.87f, 0.88f, 1.00f);
    c[ImGuiCol_TextDisabled]        = ImVec4(0.45f, 0.46f, 0.50f, 1.00f);
    c[ImGuiCol_TextSelectedBg]      = ImVec4(0.25f, 0.50f, 0.85f, 0.45f);

    // Borders / Separators
    c[ImGuiCol_Border]              = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    c[ImGuiCol_BorderShadow]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_Separator]           = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    c[ImGuiCol_SeparatorHovered]    = ImVec4(0.30f, 0.50f, 0.85f, 0.78f);
    c[ImGuiCol_SeparatorActive]     = ImVec4(0.30f, 0.50f, 0.85f, 1.00f);

    // Frames (inputs, sliders)
    c[ImGuiCol_FrameBg]             = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
    c[ImGuiCol_FrameBgHovered]      = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgActive]       = ImVec4(0.24f, 0.26f, 0.32f, 1.00f);

    // Title bar
    c[ImGuiCol_TitleBg]             = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_TitleBgActive]       = ImVec4(0.13f, 0.15f, 0.20f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.10f, 0.10f, 0.12f, 0.50f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]         = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]       = ImVec4(0.28f, 0.29f, 0.34f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.36f, 0.38f, 0.45f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.42f, 0.46f, 0.55f, 1.00f);

    // Headers / Selectable / TableHeader
    c[ImGuiCol_Header]              = ImVec4(0.18f, 0.30f, 0.50f, 0.45f);
    c[ImGuiCol_HeaderHovered]       = ImVec4(0.25f, 0.42f, 0.65f, 0.65f);
    c[ImGuiCol_HeaderActive]        = ImVec4(0.30f, 0.50f, 0.78f, 0.85f);
    c[ImGuiCol_TableHeaderBg]       = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
    c[ImGuiCol_TableBorderStrong]   = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
    c[ImGuiCol_TableBorderLight]    = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    c[ImGuiCol_TableRowBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TableRowBgAlt]       = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);

    // Buttons
    c[ImGuiCol_Button]              = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    c[ImGuiCol_ButtonHovered]       = ImVec4(0.24f, 0.30f, 0.42f, 1.00f);
    c[ImGuiCol_ButtonActive]        = ImVec4(0.30f, 0.50f, 0.78f, 1.00f);
    c[ImGuiCol_CheckMark]           = ImVec4(0.40f, 0.65f, 0.95f, 1.00f);
    c[ImGuiCol_SliderGrab]          = ImVec4(0.30f, 0.50f, 0.78f, 1.00f);
    c[ImGuiCol_SliderGrabActive]    = ImVec4(0.40f, 0.65f, 0.95f, 1.00f);

    // Tabs
    c[ImGuiCol_Tab]                 = ImVec4(0.13f, 0.13f, 0.16f, 1.00f);
    c[ImGuiCol_TabHovered]          = ImVec4(0.30f, 0.42f, 0.62f, 1.00f);
    c[ImGuiCol_TabActive]           = ImVec4(0.20f, 0.28f, 0.40f, 1.00f);
    c[ImGuiCol_TabUnfocused]        = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);

    // Docking
    c[ImGuiCol_DockingPreview]      = ImVec4(0.30f, 0.50f, 0.85f, 0.55f);

    // Resize / nav
    c[ImGuiCol_ResizeGrip]          = ImVec4(0.30f, 0.50f, 0.78f, 0.20f);
    c[ImGuiCol_ResizeGripHovered]   = ImVec4(0.30f, 0.50f, 0.78f, 0.50f);
    c[ImGuiCol_ResizeGripActive]    = ImVec4(0.30f, 0.50f, 0.78f, 0.80f);
    c[ImGuiCol_NavHighlight]        = ImVec4(0.30f, 0.50f, 0.85f, 0.80f);
}

} // namespace ui
