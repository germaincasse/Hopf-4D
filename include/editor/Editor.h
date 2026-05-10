#pragma once

#include "editor/panels/Panel.h"

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

    bool                                m_initialized = false;
    bool                                m_layoutBuilt = false;
    EditorContext                       m_ctx;
    std::vector<std::unique_ptr<Panel>> m_panels;
};

} // namespace hopf::editor
