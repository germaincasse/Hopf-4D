#pragma once

#include "audio/AudioEngine.h"
#include "editor/Editor.h"
#include "platform/Window.h"
#include "scene/Scene.h"

#include <filesystem>
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
    std::unique_ptr<audio::AudioEngine> m_audio;
    std::filesystem::path             m_projectDir;
};

} // namespace hopf::core
