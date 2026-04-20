#pragma once

#include <windows.h>
#include <string>

namespace platform {

using AddFontsFn = void (*)();

bool startup(const wchar_t* title, int width, int height,
             AddFontsFn add_fonts = nullptr);
void shutdown();

// Pumps Win32 messages; returns false after WM_QUIT.
bool pump_messages();

void begin_frame();
void present();

void request_exit();

HWND  hwnd();
float current_dpi_scale();   // 1.0 = 96 DPI; safe to call before window is up

// Returns true and fills `out` with the next queued dropped file path.
bool pop_dropped_file(std::wstring& out);

} // namespace platform
