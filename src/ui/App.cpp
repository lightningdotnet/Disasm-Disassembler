#include "App.h"

#include "DisasmView.h"
#include "ExportsView.h"
#include "GotoDialog.h"
#include "HexView.h"
#include "Icons.h"
#include "ImportsView.h"
#include "SectionsView.h"
#include "StatusBar.h"
#include "StringsView.h"
#include "Theme.h"
#include "XrefsPopup.h"

#include "platform/Win32DX11.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <windows.h>
#include <commdlg.h>

#include <cstdio>
#include <memory>
#include <string>

namespace ui {

namespace {

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                  nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                          out.data(), n, nullptr, nullptr);
    return out;
}

std::string basename_utf8(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    std::wstring base = (pos == std::wstring::npos) ? path : path.substr(pos + 1);
    return wide_to_utf8(base);
}

bool load_into_document(Document& doc, const std::wstring& path) {
    if (!doc.image.load(path)) {
        doc.status_message = "Failed to load PE";
        return false;
    }
    if (!doc.engine.init(doc.image.is_64bit())) {
        doc.status_message = "Zydis init failed";
        return false;
    }

    doc.path         = path;
    doc.display_name = basename_utf8(path);

    for (const auto& mod : doc.image.imports()) {
        for (const auto& f : mod.functions) {
            std::string full = mod.dll + "!";
            if (!f.name.empty()) full += f.name;
            else                 full += "Ordinal_" + std::to_string(f.ordinal);
            doc.symbols.add(f.iat_va, std::move(full),
                            analysis::SymbolStore::Source::ImportIat);
        }
    }
    for (const auto& e : doc.image.exports()) {
        if (e.name.empty()) continue;
        uint64_t va = doc.image.image_base() + e.rva;
        doc.symbols.add(va, e.name, analysis::SymbolStore::Source::Export);
    }

    doc.pdb.load(doc.image, doc.symbols);

    doc.index.build(doc.engine, doc.image);

    const ZydisDecoder* dec = doc.engine.decoder();
    for (const auto& e : doc.index.entries()) {
        ZydisDecodedInstruction instr;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        const uint8_t* p = doc.image.data() + e.file_off;
        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(dec, p, e.length, &instr, operands))) continue;
        if (instr.meta.category != ZYDIS_CATEGORY_CALL) continue;
        for (uint8_t i = 0; i < instr.operand_count_visible; ++i) {
            const auto& op = operands[i];
            if (op.type != ZYDIS_OPERAND_TYPE_IMMEDIATE || !op.imm.is_relative) continue;
            uint64_t abs = 0;
            ZydisCalcAbsoluteAddress(&instr, &op, e.addr, &abs);
            doc.symbols.ensure_sub(abs, doc.image.is_64bit());
            break;
        }
    }

    doc.formatter.init(&doc.symbols, doc.image.is_64bit());
    doc.strings.scan(doc.image);
    doc.xrefs.build(doc.engine, doc.index, doc.image);

    uint64_t entry = doc.image.entry_point_va();
    doc.selected_addr     = entry;
    doc.pending_scroll_va = entry;
    doc.nav.clear();
    doc.nav.go(entry);

    doc.status_message = "Loaded";
    return true;
}

} // namespace

bool open_document(AppContext& ctx, const std::wstring& path) {
    auto d = std::make_unique<Document>();
    if (!load_into_document(*d, path)) return false;
    ctx.docs.push_back(std::move(d));
    ctx.current_doc    = (int)ctx.docs.size() - 1;
    ctx.pending_select = ctx.current_doc;
    return true;
}

void close_document(AppContext& ctx, int index) {
    if (index < 0 || index >= (int)ctx.docs.size()) return;
    ctx.docs.erase(ctx.docs.begin() + index);
    if (ctx.docs.empty()) {
        ctx.current_doc = -1;
    } else if (ctx.current_doc >= (int)ctx.docs.size()) {
        ctx.current_doc = (int)ctx.docs.size() - 1;
        ctx.pending_select = ctx.current_doc;
    } else if (index <= ctx.current_doc) {
        ctx.current_doc = (ctx.current_doc > 0) ? ctx.current_doc - 1 : 0;
        ctx.pending_select = ctx.current_doc;
    }
}

// -----------------------------------------------------------------------------
bool App::prompt_open_file(std::wstring& out) {
    wchar_t path[MAX_PATH] = {};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = platform::hwnd();
    ofn.lpstrFilter = L"PE binaries\0*.exe;*.dll;*.sys;*.ocx\0All files\0*.*\0\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (!::GetOpenFileNameW(&ofn)) return false;
    out = path;
    return true;
}

// -----------------------------------------------------------------------------
int App::run(const std::wstring& initial_file) {
    if (!platform::startup(L"Disassembler", 1400, 900, &ui::load_mono_font)) return 1;
    ui::apply_theme();

    if (!initial_file.empty()) open_document(m_ctx, initial_file);

    while (platform::pump_messages()) {
        platform::begin_frame();

        handle_file_drops();

        render_menu_bar();      // first: shrinks viewport WorkSize
        render_host(m_ctx);     // host window: tabs + dockspace + status bar
        handle_shortcuts();

        render_disasm_view(m_ctx);
        render_hex_view(m_ctx);
        render_sections_view(m_ctx);
        render_imports_view(m_ctx);
        render_exports_view(m_ctx);
        render_strings_view(m_ctx);

        render_goto_dialog(m_ctx);
        render_xrefs_popup(m_ctx);

        platform::present();
    }

    platform::shutdown();
    return 0;
}

void App::handle_file_drops() {
    std::wstring path;
    while (platform::pop_dropped_file(path)) {
        open_document(m_ctx, path);
    }
}

void App::render_host(AppContext& ctx) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,    0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,  0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,     ImVec2(0, 0));

    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;

    ImGui::Begin("##host", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    // ---- Tab bar (top) ----
    int close_idx = -1;
    if (!ctx.docs.empty()) {
        const ImGuiTabBarFlags tflags = ImGuiTabBarFlags_Reorderable |
                                        ImGuiTabBarFlags_FittingPolicyScroll |
                                        ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
        if (ImGui::BeginTabBar("##doctabs", tflags)) {
            for (int i = 0; i < (int)ctx.docs.size(); ++i) {
                bool open = true;
                char label[256];
                std::snprintf(label, sizeof(label), ICON_DOC "  %s###tab%d",
                              ctx.docs[i]->display_name.c_str(), i);
                ImGuiTabItemFlags tf = (i == ctx.pending_select)
                                       ? ImGuiTabItemFlags_SetSelected : 0;
                if (ImGui::BeginTabItem(label, &open, tf)) {
                    ctx.current_doc = i;
                    ImGui::EndTabItem();
                }
                if (!open) close_idx = i;
            }
            ImGui::EndTabBar();
        }
        ctx.pending_select = -1;
    } else {
        // Subtle hint where the tab bar would be
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::Indent(10.0f);
        ImGui::TextUnformatted(ICON_OPEN "  Drop a PE here, or use File \xe2\x86\x92 Open\xe2\x80\xa6");
        ImGui::Unindent(10.0f);
        ImGui::PopStyleColor();
    }

    // ---- Dockspace (middle) — leave room for status bar ----
    const float status_h = ImGui::GetFrameHeight();
    ImVec2 dock_size = ImGui::GetContentRegionAvail();
    dock_size.y -= status_h;
    if (dock_size.y < 50.0f) dock_size.y = 50.0f;
    ImGui::DockSpace(ImGui::GetID("DockSpaceMain"), dock_size, 0);

    // ---- Status bar (bottom, in-host) ----
    ImGui::Separator();
    render_status_bar(ctx);

    ImGui::End();

    if (close_idx >= 0) close_document(ctx, close_idx);
}

void App::render_menu_bar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu(ICON_OPEN "  File")) {
        if (ImGui::MenuItem(ICON_OPEN "  Open\xe2\x80\xa6", "Ctrl+O")) {
            std::wstring path;
            if (prompt_open_file(path)) open_document(m_ctx, path);
        }
        if (ImGui::MenuItem(ICON_CLOSE "  Close current tab",
                            "Ctrl+W", false, m_ctx.has_doc())) {
            if (m_ctx.current_doc >= 0) close_document(m_ctx, m_ctx.current_doc);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) platform::request_exit();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(ICON_LIST "  View")) {
        ImGui::MenuItem("Disassembly", nullptr, &m_ctx.show_disasm);
        ImGui::MenuItem("Hex",         nullptr, &m_ctx.show_hex);
        ImGui::MenuItem("Sections",    nullptr, &m_ctx.show_sections);
        ImGui::MenuItem("Imports",     nullptr, &m_ctx.show_imports);
        ImGui::MenuItem("Exports",     nullptr, &m_ctx.show_exports);
        ImGui::MenuItem("Strings",     nullptr, &m_ctx.show_strings);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(ICON_FORWARD "  Navigate")) {
        const bool has = m_ctx.has_doc();
        Document* doc = m_ctx.current();
        if (ImGui::MenuItem(ICON_SEARCH "  Go to\xe2\x80\xa6", "G", false, has))
            m_ctx.goto_open = true;
        if (ImGui::MenuItem(ICON_LINK "  Xrefs to selection", "X", false, has)) {
            m_ctx.xrefs_open   = true;
            m_ctx.xrefs_target = doc->selected_addr;
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_BACK "  Back", "Alt+Left",
                            false, has && doc->nav.can_back())) {
            uint64_t a = doc->nav.back();
            if (a != UINT64_MAX) { doc->pending_scroll_va = a; doc->selected_addr = a; }
        }
        if (ImGui::MenuItem(ICON_FORWARD "  Forward", "Alt+Right",
                            false, has && doc->nav.can_forward())) {
            uint64_t a = doc->nav.forward();
            if (a != UINT64_MAX) { doc->pending_scroll_va = a; doc->selected_addr = a; }
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_HOME "  Entry point", nullptr, false, has)) {
            uint64_t a = doc->image.entry_point_va();
            doc->pending_scroll_va = a;
            doc->selected_addr = a;
            doc->nav.go(a);
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void App::handle_shortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;

    const bool ctrl = io.KeyCtrl;
    const bool alt  = io.KeyAlt;

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
        std::wstring path;
        if (prompt_open_file(path)) open_document(m_ctx, path);
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        if (m_ctx.current_doc >= 0) close_document(m_ctx, m_ctx.current_doc);
    }

    // Ctrl+Tab / Ctrl+Shift+Tab — switch tabs
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Tab, false) && !m_ctx.docs.empty()) {
        const int n = (int)m_ctx.docs.size();
        const int delta = io.KeyShift ? -1 : 1;
        m_ctx.current_doc = (m_ctx.current_doc + delta + n) % n;
        m_ctx.pending_select = m_ctx.current_doc;
    }

    Document* doc = m_ctx.current();
    if (!doc) return;

    if (ImGui::IsKeyPressed(ImGuiKey_G, false)) m_ctx.goto_open = true;
    if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        m_ctx.xrefs_open   = true;
        m_ctx.xrefs_target = doc->selected_addr;
    }
    if (alt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
        uint64_t a = doc->nav.back();
        if (a != UINT64_MAX) { doc->pending_scroll_va = a; doc->selected_addr = a; }
    }
    if (alt && ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
        uint64_t a = doc->nav.forward();
        if (a != UINT64_MAX) { doc->pending_scroll_va = a; doc->selected_addr = a; }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && doc->nav.can_back()) {
        uint64_t a = doc->nav.back();
        if (a != UINT64_MAX) { doc->pending_scroll_va = a; doc->selected_addr = a; }
    }
}

} // namespace ui
