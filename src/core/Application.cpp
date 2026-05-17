#include "core/Application.h"

#include "audio/AudioEngine.h"
#include "core/Logger.h"
#include "geometry/Primitives.h"
#include "physics/Physics.h"
#include "scene/Scene.h"
#include "scene/SceneSerializer.h"
#include "scripting/Script.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <vector>

namespace hopf::core {

Application::Application() {
    platform::WindowDesc desc;
    desc.title  = "Hopf";
    desc.width  = 1600;
    desc.height = 900;
    desc.vsync  = true;

    m_window = std::make_unique<platform::Window>(desc);
    m_editor = std::make_unique<editor::Editor>();

    std::error_code ecPre;
    const auto projectDir = std::filesystem::current_path(ecPre) / "Project";
    m_editor->setProjectRoot(projectDir.string());

    m_editor->init(m_window->native());

    m_audio = std::make_unique<audio::AudioEngine>();
    m_audio->init();

    m_scene = std::make_unique<scene::Scene>();
    m_scene->setName("Untitled.hopf");
    buildDefaultScene();

    // Make sure a tidy "Project" folder exists with just our default scene file,
    // so the Files panel doesn't dump the entire repo on first launch.
    std::error_code ec;
    std::filesystem::create_directories(projectDir, ec);
    const auto defaultScenePath = (projectDir / "Untitled.hopf").string();
    if (!std::filesystem::exists(defaultScenePath, ec)) {
        scene::saveScene(*m_scene, defaultScenePath);
    }
    m_projectDir = projectDir;

    Logger::info("Application initialized");
}

Application::~Application() {
    if (m_editor) m_editor->shutdown();
    m_editor.reset();
    if (m_audio) m_audio->shutdown();
    m_audio.reset();
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
    // Place the camera a few units away from the origin (where the default cube
    // sits) so it doesn't overlap the geometry it is meant to be looking at.
    cam.transform.position = {0.f, 1.f, 5.f, 0.f};
    cam.camera4D = scene::Camera4DComponent{};
    cam.camera4D->isMain = true;
    cam.audioListener = scene::AudioListenerComponent{};
}

int Application::run() {
    double lastTime = glfwGetTime();
    auto prevPlayState = editor::PlayState::Stopped;

    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        const double now = glfwGetTime();
        const float  dt  = static_cast<float>(now - lastTime);
        lastTime = now;

        // Editor-time animations: autoRotate ticks every frame, regardless of play state.
        for (auto& e : m_scene->entities()) {
            e.transform.rotation.xy += e.autoRotate.xy * dt;
            e.transform.rotation.xz += e.autoRotate.xz * dt;
            e.transform.rotation.xw += e.autoRotate.xw * dt;
            e.transform.rotation.yz += e.autoRotate.yz * dt;
            e.transform.rotation.yw += e.autoRotate.yw * dt;
            e.transform.rotation.zw += e.autoRotate.zw * dt;
        }

        const auto playState = m_editor->playState();

        // On the Stopped -> Playing edge: bind scripts, fire onStart, spawn audio sources.
        // We snapshot the (entity, script*) pairs first so that scripts which spawn
        // additional entities during onStart don't invalidate the iterator.
        if (playState == editor::PlayState::Playing
            && prevPlayState != editor::PlayState::Playing)
        {
            struct Pending { scene::EntityId id; scripting::Script* script; };
            std::vector<Pending> pending;
            for (auto& e : m_scene->entities()) {
                for (auto& s : e.scripts) {
                    if (s.instance) pending.push_back({e.id, s.instance.get()});
                }
            }
            for (auto& p : pending) {
                p.script->_bind(p.id, m_scene.get());
                p.script->onStart();
            }
            if (m_audio) m_audio->onPlayStart(*m_scene);
        }
        // On the Playing -> Stopped edge: tear down running sounds.
        if (playState == editor::PlayState::Stopped
            && prevPlayState != editor::PlayState::Stopped)
        {
            if (m_audio) m_audio->onPlayStop();
        }
        prevPlayState = playState;

        if (playState == editor::PlayState::Playing) {
            physics::step(*m_scene, dt);
            std::vector<scripting::Script*> updates;
            for (auto& e : m_scene->entities()) {
                for (auto& s : e.scripts) {
                    if (s.instance) updates.push_back(s.instance.get());
                }
            }
            for (auto* s : updates) s->onUpdate(dt);
            if (m_audio) m_audio->update(*m_scene);
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
