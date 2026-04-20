cmake_minimum_required(VERSION 3.20)
project(disasm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

# ---------------------------------------------------------------- Zydis v4
set(ZYDIS_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_TOOLS      OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_MAN        OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_DOXYGEN    OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_ENCODER  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(zydis
    GIT_REPOSITORY https://github.com/zyantific/zydis.git
    GIT_TAG        v4.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(zydis)

# ---------------------------------------------------------------- Dear ImGui (docking)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# ---------------------------------------------------------------- disasm exe
add_executable(disasm WIN32
    src/main.cpp

    src/platform/Win32DX11.cpp

    src/pe/MappedFile.cpp
    src/pe/PEImage.cpp
    src/pe/Pdb.cpp

    src/disasm/Engine.cpp
    src/disasm/DecodedIndex.cpp
    src/disasm/Formatter.cpp

    src/analysis/SymbolStore.cpp
    src/analysis/Strings.cpp
    src/analysis/Xrefs.cpp
    src/analysis/Navigation.cpp

    src/ui/App.cpp
    src/ui/Theme.cpp
    src/ui/DisasmView.cpp
    src/ui/HexView.cpp
    src/ui/SectionsView.cpp
    src/ui/ImportsView.cpp
    src/ui/ExportsView.cpp
    src/ui/StringsView.cpp
    src/ui/XrefsPopup.cpp
    src/ui/GotoDialog.cpp
    src/ui/StatusBar.cpp
)

target_include_directories(disasm PRIVATE src)

target_link_libraries(disasm PRIVATE
    Zydis
    imgui
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    shell32
    user32
    gdi32
    comdlg32
    dbghelp
)

if(MSVC)
    target_compile_options(disasm PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus /wd4100 /wd4505)
    target_compile_definitions(disasm PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(disasm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
