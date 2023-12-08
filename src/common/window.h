#pragma once
#include <cstdint>
#include <string>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace fart {

struct Window {

    public:

        Window(uint32_t width, uint32_t height, std::string window_title);
        ~Window();

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        bool shouldClose() { return glfwWindowShouldClose(m_window); }
        GLFWwindow* getGlfwWindow() { return m_window; }

        uint32_t getHeight() { return m_height; }
        uint32_t getWidth() { return m_width; }
        glm::u32vec2 getViewportSize() { return glm::u32vec2(m_width, m_height); }

        bool isWindowFocused() const { return m_window_focused; }

        glm::vec2 getMousePosition() const { return m_mouse_position; }
        glm::vec2 getPrevMousePosition() const { return m_prev_mouse_position; }
        glm::vec2 getDeltaMousePosition() const { return m_mouse_position - m_prev_mouse_position; }

        bool isMouseLeftPressed() const { return m_mouse_left_pressed; }
        bool isMouseRightPressed() const { return m_mouse_right_pressed; }
        bool isMouseMiddlePressed() const { return m_mouse_middle_pressed; }

    private:
        void windowFocusCallback(GLFWwindow* window, int focus) {
            m_window_focused = focus != 0;
        }
        void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
            m_prev_mouse_position = m_mouse_position;
            m_mouse_position.x = (xpos / m_width) * 2.f - 1.f;
            m_mouse_position.y = (ypos / m_height) * -2.f + 1.f;
        }

        void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
            m_mouse_left_pressed = action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT;
            m_mouse_right_pressed = action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT;
            m_mouse_middle_pressed = action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_MIDDLE;
        }

        void initWindow();
        
        const uint32_t m_width;
        const uint32_t m_height;

        std::string m_window_title;

        GLFWwindow *m_window;

        bool m_window_focused;
        glm::vec2 m_mouse_position;
        glm::vec2 m_prev_mouse_position;
        bool m_mouse_left_pressed { false };
        bool m_mouse_right_pressed { false };
        bool m_mouse_middle_pressed { false };
};

}
