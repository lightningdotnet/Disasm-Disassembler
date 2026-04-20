#include "Win32DX11.h"

#include <d3d11.h>
#include <dwmapi.h>
#include <dxgi.h>
#include <shellapi.h>

#include <deque>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace {

HWND                     g_hwnd        = nullptr;
ID3D11Device*            g_device      = nullptr;
ID3D11DeviceContext*     g_context     = nullptr;
IDXGISwapChain*          g_swapchain   = nullptr;
ID3D11RenderTargetView*  g_rtv         = nullptr;
UINT                     g_resize_w    = 0;
UINT                     g_resize_h    = 0;
std::deque<std::wstring> g_dropped;

const wchar_t* kClassName = L"DisasmWindow";

void create_render_target() {
    ID3D11Texture2D* back = nullptr;
    g_swapchain->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }
}

void cleanup_render_target() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

bool create_device(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL fl;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levels, (UINT)_countof(levels), D3D11_SDK_VERSION,
        &sd, &g_swapchain, &g_device, &fl, &g_context);

    if (FAILED(hr)) return false;

    create_render_target();
    return true;
}

void cleanup_device() {
    cleanup_render_target();
    if (g_swapchain) { g_swapchain->Release(); g_swapchain = nullptr; }
    if (g_context)   { g_context->Release();   g_context   = nullptr; }
    if (g_device)    { g_device->Release();    g_device    = nullptr; }
}

void apply_dark_titlebar(HWND hwnd) {
    const BOOL dark = TRUE;
    // Best-effort; ignored on older Windows.
    ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

void handle_drop(HDROP drop) {
    const UINT n = ::DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < n; ++i) {
        const UINT len = ::DragQueryFileW(drop, i, nullptr, 0);
        if (!len) continue;
        std::wstring s(len, L'\0');
        ::DragQueryFileW(drop, i, s.data(), len + 1);
        g_dropped.push_back(std::move(s));
    }
    ::DragFinish(drop);
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w, l)) return 1;

    switch (msg) {
    case WM_SIZE:
        if (w == SIZE_MINIMIZED) return 0;
        g_resize_w = (UINT)LOWORD(l);
        g_resize_h = (UINT)HIWORD(l);
        return 0;
    case WM_SYSCOMMAND:
        if ((w & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DROPFILES:
        handle_drop(reinterpret_cast<HDROP>(w));
        return 0;
    case WM_DPICHANGED:
        // Resize window to suggested rect; font atlas stays — good-enough degradation
        if (LPRECT r = reinterpret_cast<LPRECT>(l)) {
            ::SetWindowPos(hwnd, nullptr, r->left, r->top,
                           r->right - r->left, r->bottom - r->top,
                           SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, w, l);
}

} // anonymous namespace

namespace platform {

bool startup(const wchar_t* title, int width, int height, AddFontsFn add_fonts) {
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = ::GetModuleHandleW(nullptr);
    wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    ::RegisterClassExW(&wc);

    g_hwnd = ::CreateWindowExW(
        0, kClassName, title, WS_OVERLAPPEDWINDOW,
        100, 100, width, height,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) return false;

    apply_dark_titlebar(g_hwnd);

    if (!create_device(g_hwnd)) {
        cleanup_device();
        ::DestroyWindow(g_hwnd);
        ::UnregisterClassW(kClassName, wc.hInstance);
        g_hwnd = nullptr;
        return false;
    }

    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);
    ::DragAcceptFiles(g_hwnd, TRUE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "disasm.ini";

    if (add_fonts) add_fonts();
    else            io.Fonts->AddFontDefault();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);
    return true;
}

void shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanup_device();
    if (g_hwnd) {
        ::DestroyWindow(g_hwnd);
        g_hwnd = nullptr;
    }
    ::UnregisterClassW(kClassName, ::GetModuleHandleW(nullptr));
}

bool pump_messages() {
    MSG msg;
    while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) return false;
    }

    if (g_resize_w != 0 && g_resize_h != 0) {
        cleanup_render_target();
        g_swapchain->ResizeBuffers(0, g_resize_w, g_resize_h, DXGI_FORMAT_UNKNOWN, 0);
        g_resize_w = g_resize_h = 0;
        create_render_target();
    }
    return true;
}

void begin_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void present() {
    ImGui::Render();

    const float clear[4] = { 0.06f, 0.06f, 0.08f, 1.0f };
    g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
    g_context->ClearRenderTargetView(g_rtv, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_swapchain->Present(1, 0);
}

void request_exit() { ::PostQuitMessage(0); }

HWND hwnd() { return g_hwnd; }

float current_dpi_scale() {
    if (!g_hwnd) return 1.0f;
    UINT dpi = ::GetDpiForWindow(g_hwnd);
    if (dpi == 0) return 1.0f;
    return static_cast<float>(dpi) / 96.0f;
}

bool pop_dropped_file(std::wstring& out) {
    if (g_dropped.empty()) return false;
    out = std::move(g_dropped.front());
    g_dropped.pop_front();
    return true;
}

} // namespace platform
