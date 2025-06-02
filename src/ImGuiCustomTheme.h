#pragma once

#include <rlImGui.h>
#include <imgui.h>

inline void ImCustomTheme()
{
    // Load a custom font
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 20.0f);
    rlImGuiReloadFonts();

    // Apply a simple dark-gray theme to frame backgrounds and rounding
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_FrameBg]        = ImColor(0.22f, 0.22f, 0.22f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImColor(0.20f, 0.20f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]  = ImColor(0.30f, 0.30f, 0.30f, 1.0f);
    style.WindowRounding                  = 5.0f;
}