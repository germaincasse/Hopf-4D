#include "hopf/core/Application.h"

#include "hopf/core/Logger.h"
#include "hopf/geometry/Primitives.h"

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
}

int Application::run() {
    double lastTime = glfwGetTime();

    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        const double now = glfwGetTime();
        const float  dt  = static_cast<float>(now - lastTime);
        lastTime = now;

        for (auto& e : m_scene->entities()) {
            e.transform.rotation.xy += e.autoRotate.xy * dt;
            e.transform.rotation.xz += e.autoRotate.xz * dt;
            e.transform.rotation.xw += e.autoRotate.xw * dt;
            e.transform.rotation.yz += e.autoRotate.yz * dt;
            e.transform.rotation.yw += e.autoRotate.yw * dt;
            e.transform.rotation.zw += e.autoRotate.zw * dt;
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
