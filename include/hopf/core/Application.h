#pragma once

#include "hopf/editor/Editor.h"
#include "hopf/platform/Window.h"
#include "hopf/scene/Scene.h"

#include <memory>

namespace hopf::core {

class Application {
public:
    Application();
    ~Application();

    int run();

private:
    void buildDefaultScene();

    std::unique_ptr<platform::Window> m_window;
    std::unique_ptr<editor::Editor>   m_editor;
    std::unique_ptr<scene::Scene>     m_scene;
};

} // namespace hopf::core
