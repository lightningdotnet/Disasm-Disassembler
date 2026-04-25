# disasm

A tiny Windows disassembler. Drag a `.exe` onto it and poke around. Single 1.3 MB exe, nothing to install, no DLLs to ship.

Runs on [Zydis](https://github.com/zyantific/zydis) for decoding and [Dear ImGui](https://github.com/ocornut/imgui) for the UI. Handles x86 and x64 PEs — it auto-detects which.

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)

## Build

You'll need CMake and MSVC (Visual Studio 2022 or newer will do). First configure pulls Zydis and ImGui for you — no manual fetching.

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

 Builds to: `build/Release/disasm.exe`.

## Use it

Run it and drop a PE on the window, or pass the path on the command line.

| | |
| --- | --- |
| `Ctrl+O` | open a file |
| `Ctrl+W` | close tab |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | cycle tabs |
| `G` | goto address |
| `X` | xrefs to current selection |
| `Alt+←` / `Esc` | back |
| `Alt+→` | forward |
| double-click a branch | follow it |

## What's in the box

- Multi-tab — open several binaries at once, each with its own state
- Full PE parser — headers, sections, imports, exports, debug dir
- PDB symbols when a matching one is around (via `dbghelp.dll`)
- Hex view synced with the disasm cursor
- ASCII + UTF-16 strings, cross-references, back/forward history
- A dark theme that actually commits to being dark — title bar too

## License

MIT. See [LICENSE](LICENSE).
