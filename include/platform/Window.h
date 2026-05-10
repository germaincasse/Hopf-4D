#pragma once

#include <string>

struct GLFWwindow;

namespace hopf::platform {

struct WindowDesc {
    int         width  = 1600;
    int         height = 900;
    std::string title  = "Hopf";
    bool        vsync  = true;
};

class Window {
public:
    explicit Window(const WindowDesc& desc);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void requestClose();
    void pollEvents();
    void swapBuffers();

    int  width()  const;
    int  height() const;
    void framebufferSize(int& outW, int& outH) const;

    GLFWwindow* native() const { return m_window; }

private:
    GLFWwindow* m_window = nullptr;
};

} // namespace hopf::platform
