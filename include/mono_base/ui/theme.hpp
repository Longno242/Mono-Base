#pragma once

#include <imgui.h>

namespace mono_base::ui::theme {

inline void apply_modern() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    s.WindowRounding = 10.0f;
    s.ChildRounding = 8.0f;
    s.FrameRounding = 6.0f;
    s.PopupRounding = 8.0f;
    s.ScrollbarRounding = 8.0f;
    s.GrabRounding = 6.0f;
    s.TabRounding = 6.0f;

    s.WindowPadding = ImVec2(14.0f, 14.0f);
    s.FramePadding = ImVec2(10.0f, 6.0f);
    s.ItemSpacing = ImVec2(10.0f, 8.0f);
    s.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    s.ScrollbarSize = 14.0f;
    s.GrabMinSize = 12.0f;
    s.TabBarBorderSize = 0.0f;
    s.WindowBorderSize = 0.0f;
    s.ChildBorderSize = 1.0f;

    const ImVec4 bg_dark     = { 0.07f, 0.07f, 0.09f, 0.97f };
    const ImVec4 bg_panel    = { 0.11f, 0.11f, 0.14f, 1.00f };
    const ImVec4 bg_element  = { 0.16f, 0.16f, 0.20f, 1.00f };
    const ImVec4 accent      = { 0.55f, 0.36f, 0.96f, 1.00f };
    const ImVec4 accent_dim  = { 0.40f, 0.26f, 0.72f, 1.00f };
    const ImVec4 text_main = { 0.94f, 0.94f, 0.97f, 1.00f };
    const ImVec4 text_dim  = { 0.55f, 0.56f, 0.62f, 1.00f };

    c[ImGuiCol_Text] = text_main;
    c[ImGuiCol_TextDisabled] = text_dim;
    c[ImGuiCol_WindowBg] = bg_dark;
    c[ImGuiCol_ChildBg] = bg_panel;
    c[ImGuiCol_PopupBg] = { 0.09f, 0.09f, 0.11f, 0.98f };
    c[ImGuiCol_Border] = { 0.22f, 0.22f, 0.28f, 0.55f };
    c[ImGuiCol_FrameBg] = bg_element;
    c[ImGuiCol_FrameBgHovered] = { 0.20f, 0.20f, 0.26f, 1.00f };
    c[ImGuiCol_FrameBgActive] = { 0.24f, 0.24f, 0.30f, 1.00f };
    c[ImGuiCol_TitleBg] = { 0.08f, 0.08f, 0.10f, 1.00f };
    c[ImGuiCol_TitleBgActive] = { 0.14f, 0.12f, 0.20f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.08f, 0.08f, 0.10f, 0.75f };
    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_SliderGrab] = accent_dim;
    c[ImGuiCol_SliderGrabActive] = accent;
    c[ImGuiCol_Button] = { 0.22f, 0.18f, 0.32f, 1.00f };
    c[ImGuiCol_ButtonHovered] = accent_dim;
    c[ImGuiCol_ButtonActive] = accent;
    c[ImGuiCol_Header] = { 0.22f, 0.18f, 0.32f, 0.80f };
    c[ImGuiCol_HeaderHovered] = accent_dim;
    c[ImGuiCol_HeaderActive] = accent;
    c[ImGuiCol_Separator] = { 0.24f, 0.24f, 0.30f, 0.60f };
    c[ImGuiCol_Tab] = { 0.12f, 0.12f, 0.16f, 1.00f };
    c[ImGuiCol_TabHovered] = accent_dim;
    c[ImGuiCol_TabSelected] = { 0.28f, 0.20f, 0.44f, 1.00f };
    c[ImGuiCol_TabDimmed] = { 0.10f, 0.10f, 0.13f, 1.00f };
    c[ImGuiCol_TabDimmedSelected] = { 0.18f, 0.14f, 0.28f, 1.00f };
    c[ImGuiCol_ScrollbarBg] = bg_panel;
    c[ImGuiCol_ScrollbarGrab] = bg_element;
    c[ImGuiCol_ScrollbarGrabHovered] = accent_dim;
    c[ImGuiCol_ScrollbarGrabActive] = accent;
}

} // namespace mono_base::ui::theme
