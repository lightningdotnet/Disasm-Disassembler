#pragma once

#include "analysis/Navigation.h"
#include "analysis/Strings.h"
#include "analysis/SymbolStore.h"
#include "analysis/Xrefs.h"
#include "disasm/DecodedIndex.h"
#include "disasm/Engine.h"
#include "disasm/Formatter.h"
#include "pe/PEImage.h"
#include "pe/Pdb.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ui {

// One open binary. Each tab owns one Document.
struct Document {
    pe::PEImage           image;
    pe::PdbLoader         pdb;
    disasm::Engine        engine;
    disasm::DecodedIndex  index;
    disasm::Formatter     formatter;
    analysis::SymbolStore symbols;
    analysis::Strings     strings;
    analysis::Xrefs       xrefs;
    analysis::Navigation  nav;

    std::wstring path;          // full path
    std::string  display_name;  // basename (UTF-8)
    std::string  status_message;

    uint64_t selected_addr     = 0;
    uint64_t pending_scroll_va = UINT64_MAX;
};

// App-level state: list of docs, selection, and global UI prefs/modals.
struct AppContext {
    std::vector<std::unique_ptr<Document>> docs;
    int  current_doc    = -1;
    int  pending_select = -1;  // one-shot tab-switch trigger

    Document* current() {
        return (current_doc >= 0 && current_doc < (int)docs.size())
            ? docs[current_doc].get() : nullptr;
    }
    const Document* current() const {
        return (current_doc >= 0 && current_doc < (int)docs.size())
            ? docs[current_doc].get() : nullptr;
    }
    bool has_doc() const { return current() != nullptr; }

    // global view toggles
    bool show_disasm    = true;
    bool show_hex       = true;
    bool show_sections  = true;
    bool show_imports   = true;
    bool show_exports   = false;
    bool show_strings   = false;

    // global modals
    bool     goto_open    = false;
    bool     xrefs_open   = false;
    uint64_t xrefs_target = 0;
};

// Implemented in App.cpp
bool open_document(AppContext& ctx, const std::wstring& path);
void close_document(AppContext& ctx, int index);

} // namespace ui
