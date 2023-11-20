#pragma once
#include <string>

#include <GLFW/glfw3.h>

namespace fart {

struct Window {

    public:
        Window(int width, int height, std::string window_title);
        ~Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        bool shouldClose() { return glfwWindowShouldClose(m_window); }

    private:
        void initWindow();
        
        const int m_width;
        const int m_height;

        std::string m_window_title;

        GLFWwindow *m_window;
};

}
