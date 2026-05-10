#include "editor/panels/FilesPanel.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <system_error>
#include <vector>

namespace hopf::editor {

namespace {

bool isIgnored(const std::filesystem::path& p) {
    const auto& name = p.filename().string();
    return name == "build" || name == ".git" || name == ".vs" ||
           name == "out"   || name == ".idea" || name == ".vscode";
}

} // namespace

FilesPanel::FilesPanel() {
    std::error_code ec;
    m_root = std::filesystem::current_path(ec);
    m_rootBuf = m_root.string();
}

void FilesPanel::render(EditorContext& /*ctx*/) {
    if (!m_open) return;
    if (!ImGui::Begin(name(), &m_open)) {
        ImGui::End();
        return;
    }

    char buf[1024];
    std::snprintf(buf, sizeof(buf), "%s", m_rootBuf.c_str());
    ImGui::SetNextItemWidth(-90.f);
    if (ImGui::InputText("##path", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
        m_rootBuf = buf;
        std::error_code ec;
        std::filesystem::path candidate(m_rootBuf);
        if (std::filesystem::is_directory(candidate, ec)) {
            m_root = candidate;
        }
    }
    ImGui::SameLine();
    ImGui::Button("Refresh");

    ImGui::Separator();

    std::error_code ec;
    if (!std::filesystem::is_directory(m_root, ec)) {
        ImGui::TextDisabled("'%s' is not a directory.", m_root.string().c_str());
        ImGui::End();
        return;
    }

    if (ImGui::BeginChild("##filesList", ImVec2(0, 0))) {
        renderEntry(m_root);
    }
    ImGui::EndChild();

    ImGui::End();
}

void FilesPanel::renderEntry(const std::filesystem::path& path) {
    std::error_code ec;
    std::vector<std::filesystem::directory_entry> dirs;
    std::vector<std::filesystem::directory_entry> files;
    for (auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (isIgnored(entry.path())) continue;
        if (entry.is_directory(ec)) dirs.push_back(entry);
        else                        files.push_back(entry);
    }
    auto byName = [](const auto& a, const auto& b) {
        return a.path().filename() < b.path().filename();
    };
    std::sort(dirs.begin(),  dirs.end(),  byName);
    std::sort(files.begin(), files.end(), byName);

    for (const auto& d : dirs) {
        const std::string label = "[ ] " + d.path().filename().string();
        if (ImGui::TreeNode(label.c_str())) {
            renderEntry(d.path());
            ImGui::TreePop();
        }
    }
    for (const auto& f : files) {
        ImGui::TreeNodeEx(f.path().filename().string().c_str(),
                          ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                          ImGuiTreeNodeFlags_SpanAvailWidth);
    }
}

} // namespace hopf::editor
