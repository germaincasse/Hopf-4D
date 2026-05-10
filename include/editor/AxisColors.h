#pragma once

#include <imgui.h>

namespace hopf::editor {

// Shared color code for the four axes (matches the gizmo lines drawn by ViewportRenderer).
inline constexpr ImVec4 kAxisColorX = ImVec4(1.00f, 0.30f, 0.30f, 1.0f);
inline constexpr ImVec4 kAxisColorY = ImVec4(0.30f, 1.00f, 0.30f, 1.0f);
inline constexpr ImVec4 kAxisColorZ = ImVec4(0.30f, 0.55f, 1.00f, 1.0f);
inline constexpr ImVec4 kAxisColorW = ImVec4(1.00f, 0.85f, 0.20f, 1.0f);

inline ImVec4 axisColor(char c) {
    switch (c) {
        case 'x': case 'X': return kAxisColorX;
        case 'y': case 'Y': return kAxisColorY;
        case 'z': case 'Z': return kAxisColorZ;
        case 'w': case 'W': return kAxisColorW;
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

inline bool isAxisChar(char c) {
    return c == 'x' || c == 'X' || c == 'y' || c == 'Y'
        || c == 'z' || c == 'Z' || c == 'w' || c == 'W';
}

// Renders a string letter-by-letter, coloring axis letters (x/y/z/w) and leaving the
// rest in the default text color. No SameLine before the first character; caller
// controls layout around the text.
inline void textAxisColored(const char* str) {
    for (const char* c = str; *c; ++c) {
        const bool axis = isAxisChar(*c);
        if (axis) ImGui::PushStyleColor(ImGuiCol_Text, axisColor(*c));
        const char buf[2] = {*c, '\0'};
        ImGui::TextUnformatted(buf);
        if (axis) ImGui::PopStyleColor();
        if (*(c + 1)) ImGui::SameLine(0.f, 0.f);
    }
}

} // namespace hopf::editor
