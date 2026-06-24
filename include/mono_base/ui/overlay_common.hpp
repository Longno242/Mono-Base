#pragma once

#include <imgui.h>

#include <Windows.h>

namespace mono_base::ui {

[[nodiscard]] inline bool is_input_message(UINT msg) {
    switch (msg) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_INPUT:
        return true;
    default:
        return false;
    }
}

inline void apply_menu_input_state(HWND hwnd, bool menu_open) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = menu_open;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (menu_open) {
        ClipCursor(nullptr);
        if (hwnd) {
            SetCapture(hwnd);
        }
    } else if (hwnd && GetCapture() == hwnd) {
        ReleaseCapture();
    }
}

} // namespace mono_base::ui
