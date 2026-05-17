#pragma once

#include "editor/panels/Panel.h"
#include "scene/Entity.h"

#include <imgui.h>

#include <memory>
#include <vector>

struct GLFWwindow;

namespace hopf::scene { class Scene; }

namespace hopf::editor {

class Editor {
public:
    Editor() = default;
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void init(GLFWwindow* window);
    void shutdown();

    // Optional initial root for the Files panel. Call before init() ideally, or
    // before the first frame to take effect on the first display.
    void setProjectRoot(const std::string& path) { m_initialProjectRoot = path; }

    void beginFrame();
    void endFrame();
    void renderUI(scene::Scene& scene);

    PlayState playState() const { return m_ctx.playState; }

private:
    void buildDefaultLayout(unsigned int dockspaceId);
    void processShortcuts();
    void applyUiScale();

    void snapshotScene(scene::Scene& scene);
    void restoreScene(scene::Scene& scene);

    bool                                m_initialized   = false;
    bool                                m_layoutBuilt   = false;
    bool                                m_openSaveDlg   = false;
    bool                                m_openLoadDlg   = false;
    char                                m_filePath[260] = "scene.hopf";
    EditorContext                       m_ctx;
    std::vector<std::unique_ptr<Panel>> m_panels;
    int                                 m_nextViewportId = 1;
    std::vector<scene::Entity>          m_sceneSnapshot;

    // Snapshot of the configured style at unit scale; ScaleAllSizes is multiplicative,
    // so we restart from this baseline whenever m_uiScale changes.
    ImGuiStyle                          m_baseStyle{};
    float                               m_uiScale = 1.f;

    std::string                         m_initialProjectRoot;
};

} // namespace hopf::editor
