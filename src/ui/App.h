#pragma once

#include "AppContext.h"

#include <string>

namespace ui {

class App {
public:
    int run(const std::wstring& initial_file = L"");

private:
    void render_menu_bar();
    void render_host(AppContext& ctx);
    void handle_shortcuts();
    void handle_file_drops();
    bool prompt_open_file(std::wstring& out);

    AppContext m_ctx;
};

} // namespace ui
