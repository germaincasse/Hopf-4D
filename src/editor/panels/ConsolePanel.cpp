#include "hopf/editor/panels/ConsolePanel.h"

#include "hopf/core/Logger.h"

#include <imgui.h>

namespace hopf::editor {

void ConsolePanel::render(EditorContext& /*ctx*/) {
    if (!m_open) return;
    if (!ImGui::Begin(name(), &m_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear")) {
        hopf::core::Logger::clear();
    }
    ImGui::SameLine();
    static bool autoScroll = true;
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();
    if (ImGui::BeginChild("##log", ImVec2(0, 0), ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        const auto& entries = hopf::core::Logger::entries();
        for (const auto& entry : entries) {
            ImVec4 col;
            switch (entry.level) {
                case hopf::core::LogLevel::Info:    col = ImVec4(0.85f, 0.85f, 0.85f, 1); break;
                case hopf::core::LogLevel::Warning: col = ImVec4(1.00f, 0.85f, 0.30f, 1); break;
                case hopf::core::LogLevel::Error:   col = ImVec4(1.00f, 0.45f, 0.45f, 1); break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(entry.message.c_str());
            ImGui::PopStyleColor();
        }
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.f);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace hopf::editor
