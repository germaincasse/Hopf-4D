#include "platform/Window.h"

#include "core/Logger.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace hopf::platform {

namespace {
int g_initCount = 0;

void glfwErrorCallback(int code, const char* description) {
    hopf::core::Logger::error("GLFW error %d: %s", code, description ? description : "(null)");
}

void ensureGlfw() {
    if (g_initCount == 0) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            throw std::runtime_error("glfwInit failed");
        }
    }
    ++g_initCount;
}

void releaseGlfw() {
    --g_initCount;
    if (g_initCount == 0) {
        glfwTerminate();
    }
}
} // namespace

Window::Window(const WindowDesc& desc) {
    ensureGlfw();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        releaseGlfw();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(desc.vsync ? 1 : 0);

    if (!gladLoaderLoadGL()) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        releaseGlfw();
        throw std::runtime_error("Failed to load OpenGL via glad");
    }

    hopf::core::Logger::info("OpenGL %s | %s | GLSL %s",
        glGetString(GL_VERSION),
        glGetString(GL_RENDERER),
        glGetString(GL_SHADING_LANGUAGE_VERSION));
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    releaseGlfw();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(m_window) != 0; }
void Window::requestClose()      { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }
void Window::pollEvents()        { glfwPollEvents(); }
void Window::swapBuffers()       { glfwSwapBuffers(m_window); }

int Window::width() const  { int w, h; glfwGetWindowSize(m_window, &w, &h); return w; }
int Window::height() const { int w, h; glfwGetWindowSize(m_window, &w, &h); return h; }

void Window::framebufferSize(int& outW, int& outH) const {
    glfwGetFramebufferSize(m_window, &outW, &outH);
}

} // namespace hopf::platform
