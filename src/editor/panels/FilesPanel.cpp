#include "editor/panels/FilesPanel.h"

#include "core/Logger.h"
#include "scene/Scene.h"
#include "scene/SceneSerializer.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <system_error>
#include <vector>

namespace hopf::editor {

namespace {

bool isIgnored(const std::filesystem::path& p) {
    const auto& name = p.filename().string();
    return name == "build" || name == ".git" || name == ".vs" ||
           name == "out"   || name == ".idea" || name == ".vscode";
}

bool hasExtension(const std::filesystem::path& p, const char* ext) {
    auto e = p.extension().string();
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return char(std::tolower(c)); });
    return e == ext;
}

// Reveal a file in the OS file manager. On Windows uses Explorer's /select to
// highlight the file. Falls back to opening the parent directory.
void revealInExplorer(const std::filesystem::path& p) {
#if defined(_WIN32)
    std::string cmd = "explorer /select,\"";
    cmd += p.string();
    cmd += "\"";
    std::system(cmd.c_str());
#elif defined(__APPLE__)
    std::string cmd = "open -R \"" + p.string() + "\"";
    std::system(cmd.c_str());
#else
    // Best-effort: open the parent dir.
    std::string cmd = "xdg-open \"" + p.parent_path().string() + "\"";
    std::system(cmd.c_str());
#endif
}

} // namespace

FilesPanel::FilesPanel() {
    std::error_code ec;
    m_root = std::filesystem::current_path(ec);
    m_rootBuf = m_root.string();
}

void FilesPanel::setRoot(const std::string& path) {
    m_rootBuf = path;
    m_root    = std::filesystem::path(path);
}

void FilesPanel::render(EditorContext& ctx) {
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
        renderEntry(m_root, ctx);
    }
    ImGui::EndChild();

    ImGui::End();
}

void FilesPanel::renderEntry(const std::filesystem::path& path, EditorContext& ctx) {
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
        ImGui::PushID(d.path().string().c_str());
        const std::string label = "[ ] " + d.path().filename().string();
        const bool open = ImGui::TreeNode(label.c_str());
        if (ImGui::BeginPopupContextItem("##dirCtx")) {
            if (ImGui::MenuItem("Open in explorer")) revealInExplorer(d.path());
            ImGui::EndPopup();
        }
        if (open) {
            renderEntry(d.path(), ctx);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    for (const auto& f : files) {
        ImGui::PushID(f.path().string().c_str());
        ImGui::TreeNodeEx(f.path().filename().string().c_str(),
                          ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                          ImGuiTreeNodeFlags_SpanAvailWidth);

        // Double-click a .hopf scene file to load it. Right-click for actions.
        if (hasExtension(f.path(), ".hopf") && ImGui::IsItemHovered()
            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ctx.scene)
        {
            ctx.undoStack.clear();
            ctx.selected = 0;
            if (scene::loadScene(*ctx.scene, f.path().string())) {
                core::Logger::info("Loaded scene from %s", f.path().string().c_str());
            }
        }
        if (ImGui::BeginPopupContextItem("##fileCtx")) {
            if (hasExtension(f.path(), ".hopf") && ImGui::MenuItem("Open scene") && ctx.scene) {
                ctx.undoStack.clear();
                ctx.selected = 0;
                if (scene::loadScene(*ctx.scene, f.path().string())) {
                    core::Logger::info("Loaded scene from %s", f.path().string().c_str());
                }
            }
            if (ImGui::MenuItem("Open file location")) revealInExplorer(f.path());
            if (ImGui::MenuItem("Copy path")) {
                ImGui::SetClipboardText(f.path().string().c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
}

} // namespace hopf::editor
