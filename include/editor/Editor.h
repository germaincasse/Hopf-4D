#pragma once

#include "editor/panels/Panel.h"

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

    void beginFrame();
    void endFrame();
    void renderUI(scene::Scene& scene);

private:
    void buildDefaultLayout(unsigned int dockspaceId);
    void processShortcuts();
    void applyUiScale();

    bool                                m_initialized = false;
    bool                                m_layoutBuilt = false;
    EditorContext                       m_ctx;
    std::vector<std::unique_ptr<Panel>> m_panels;
    int                                 m_nextViewportId = 1;

    // Snapshot of the configured style at unit scale; ScaleAllSizes is multiplicative,
    // so we restart from this baseline whenever m_uiScale changes.
    ImGuiStyle                          m_baseStyle{};
    float                               m_uiScale = 1.f;
};

} // namespace hopf::editor
