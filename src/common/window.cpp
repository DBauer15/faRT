#include "window.h"
#include <GLFW/glfw3.h>

namespace fart {

Window::Window(int w, int h, std::string title) : m_width(w), m_height(h), m_window_title(title) {
    initWindow();
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void
Window::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(m_width, m_height, m_window_title.c_str(), nullptr, nullptr);
}

}
