#include "core/Application.h"

#include "core/Logger.h"
#include "geometry/Primitives.h"
#include "physics/Physics.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace hopf::core {

Application::Application() {
    platform::WindowDesc desc;
    desc.title  = "Hopf";
    desc.width  = 1600;
    desc.height = 900;
    desc.vsync  = true;

    m_window = std::make_unique<platform::Window>(desc);
    m_editor = std::make_unique<editor::Editor>();
    m_editor->init(m_window->native());

    m_scene = std::make_unique<scene::Scene>();
    m_scene->setName("Untitled.hopf");
    buildDefaultScene();

    Logger::info("Application initialized");
}

Application::~Application() {
    if (m_editor) m_editor->shutdown();
    m_editor.reset();
    m_window.reset();
}

void Application::buildDefaultScene() {
    m_scene->addPrimitive(geometry::PrimitiveType::Tesseract);

    auto& sun = m_scene->addEntity("Sun");
    sun.light = scene::DirectionalLight{};
    sun.transform.position = {2.f, 2.f, 0.f, 0.f};
    // Tilt the canonical (0, -1, 0, 0) direction toward the front-right for a typical
    // sun angle. The light direction is derived from this rotation at render time.
    sun.transform.rotation.xy = 0.4f;
    sun.transform.rotation.yz = 0.5f;

    auto& cam = m_scene->addEntity("Main Camera");
    cam.camera4D = scene::Camera4DComponent{};
    cam.camera4D->isMain = true;
}

int Application::run() {
    double lastTime = glfwGetTime();

    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        const double now = glfwGetTime();
        const float  dt  = static_cast<float>(now - lastTime);
        lastTime = now;

        if (m_editor->playState() == editor::PlayState::Playing) {
            physics::step(*m_scene, dt);
        }

        int fbW = 0, fbH = 0;
        m_window->framebufferSize(fbW, fbH);
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.08f, 0.09f, 0.10f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_editor->beginFrame();
        m_editor->renderUI(*m_scene);
        m_editor->endFrame();

        m_window->swapBuffers();
    }
    return 0;
}

} // namespace hopf::core
