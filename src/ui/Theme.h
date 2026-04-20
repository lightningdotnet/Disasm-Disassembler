#pragma once

namespace ui {

// Refined dark theme; safe to call multiple times.
void apply_theme();

// Loads Consolas + merges Segoe MDL2 Assets icon font.
// Sizing scales with the current monitor DPI.
void load_mono_font();

} // namespace ui
