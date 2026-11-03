#!/usr/bin/env python3
"""Fix initial touch insets, direct touch menu input, and bundled CJK font lookup."""
from pathlib import Path
gpu = Path("upstream-runtime/gpu/shim")
p = gpu / "bbport_touch_controls.h"
s = p.read_text()
s = s.replace("namespace BbTouch {", "namespace BbTouch {\n\n/// Window thread: initialize safe-area geometry before any controls are drawn.\nvoid ConfigureWindow(SDL_WindowID id);", 1)
p.write_text(s)
p = gpu / "bbport_touch_controls.cpp"
s = p.read_text()
needle = "bool Shown() {"
assert s.count(needle) == 1
s = s.replace(needle, "void ConfigureWindow(SDL_WindowID id) {\n    std::scoped_lock lock{mutex};\n    UpdateShape(id);\n}\n\n" + needle)
p.write_text(s)
p = gpu / "window.cpp"
s = p.read_text()
needle = '    ASSERT_MSG(window, "Failed to create window: {}", SDL_GetError());'
assert s.count(needle) == 1
s = s.replace(needle, needle + "\n    BbTouch::ConfigureWindow(SDL_GetWindowID(window));")
needle = "    SDL_Event event;\n    while (SDL_PollEvent(&event)) {"
assert s.count(needle) == 1
s = s.replace(needle, "    BbTouch::ConfigureWindow(SDL_GetWindowID(window));\n" + needle)
p.write_text(s)
p = gpu / "bbport_overlay.cpp"
s = p.read_text()
needle = "    switch (event.type) {\n    case SDL_EVENT_KEY_DOWN:"
assert s.count(needle) == 1
s = s.replace(needle, r'''
    static SDL_FingerID menu_finger = 0;
    static bool menu_finger_down = false;
    switch (event.type) {
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED: {
        if (!is_open) {
            menu_finger_down = false;
            return false;
        }
        const bool down = event.type == SDL_EVENT_FINGER_DOWN;
        if (down) {
            if (menu_finger_down) return true;
            menu_finger = event.tfinger.fingerID;
            menu_finger_down = true;
        }
        if (!menu_finger_down || event.tfinger.fingerID != menu_finger) return true;
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent(event.tfinger.x * io.DisplaySize.x, event.tfinger.y * io.DisplaySize.y);
        if (down) io.AddMouseButtonEvent(0, true);
        if (event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED) {
            io.AddMouseButtonEvent(0, false);
            menu_finger_down = false;
        }
        return true;
    }
    case SDL_EVENT_KEY_DOWN:''')
needle = "    case SDL_EVENT_MOUSE_MOTION: {\n        if (!is_open"
assert s.count(needle) == 1
s = s.replace(needle, "    case SDL_EVENT_MOUSE_MOTION: {\n        if (is_open && event.motion.which == SDL_TOUCH_MOUSEID) return true;\n        if (!is_open")
needle = "    case SDL_EVENT_MOUSE_BUTTON_UP: {\n        if (!is_open"
assert s.count(needle) == 1
s = s.replace(needle, "    case SDL_EVENT_MOUSE_BUTTON_UP: {\n        if (is_open && event.button.which == SDL_TOUCH_MOUSEID) return true;\n        if (!is_open")
needle = "    ImGui::PopStyleVar();\n    help_text = nullptr;"
assert s.count(needle) == 1
s = s.replace(needle, r'''
    ImGui::PopStyleVar();
#if defined(__APPLE__) && TARGET_OS_IPHONE
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(),
        ImGui::GetWindowContentRegionMax().x - S(180)));
    if (ImGui::Button("Close / 닫기", ImVec2(S(180), S(64)))) {
        SetOpen(false);
        ImGui::End();
        return;
    }
    ImGui::Separator();
#endif
    help_text = nullptr;''')
needle = "        std::vector<const char*> candidates;"
assert s.count(needle) == 1
s = s.replace(needle, r'''
        if (const char* bundled = std::getenv("BB_CJK_FONT"); bundled && *bundled) {
            ImFontConfig config;
            config.MergeMode = true;
            std::error_code error;
            if (std::filesystem::exists(bundled, error) &&
                io.Fonts->AddFontFromFileTTF(bundled, 18.0f, &config)) {
                std::printf("Menu: bundled CJK font loaded: %s\n", bundled);
                return;
            }
            std::fprintf(stderr, "Menu: bundled CJK font could not load: %s\n", bundled);
        }
        std::vector<const char*> candidates;''')
p.write_text(s)
print("Touch/menu: pre-input safe-area geometry, direct finger events, close button, bundled CJK font")
