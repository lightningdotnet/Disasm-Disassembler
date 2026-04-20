#include "ui/App.h"

#include <windows.h>
#include <shellapi.h>

#include <string>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::wstring initial;
    int    argc = 0;
    LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv && argc >= 2) initial = argv[1];
    if (argv) ::LocalFree(argv);

    ui::App app;
    return app.run(initial);
}
