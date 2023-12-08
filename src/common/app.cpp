#include "app.h"
#include "scene.h"
#include <memory>
#include <iostream>
#include <string>

namespace fart {

App::App(std::string scene) {
    m_renderer = std::make_unique<DeviceRenderer>();
    m_scene = std::make_unique<Scene>(scene);
    m_camera = std::make_unique<Camera>(
            glm::vec3( 0.f, 0.f, -m_scene->getSceneScale() ), 
            glm::vec3( 0.f, 0.f, 0.f ), 
            glm::vec3( 0.f, 1.f, 0.f ));
    m_window = std::make_shared<Window>(WIDTH, HEIGHT, "FaRT - " + scene + " @ " + m_renderer->name());

    m_renderer->init(m_scene, m_window);

    SUCC("Finished initializing renderer (" + m_renderer->name() + ")");
}

void
App::run() {

    while(!m_window->shouldClose()) {
        if (m_window->isWindowFocused()) {
            // camera update
            if (m_window->isMouseLeftPressed())
                m_camera->rotate(m_window->getPrevMousePosition(), m_window->getMousePosition());
            if (m_window->isMouseRightPressed())
                m_camera->zoom(m_window->getDeltaMousePosition().y * m_scene->getSceneScale());
            if (m_window->isMouseMiddlePressed())
                m_camera->pan(m_window->getDeltaMousePosition());
        }

        // render pass
        m_renderer->render(m_camera->eye(), m_camera->dir(), m_camera->up());

        glfwPollEvents();
    }
}

}
