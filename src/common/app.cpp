#include "app.h"
#include "scene.h"
#include <memory>
#include <iostream>
#include <string>

namespace fart {

App::App(std::string scene) {

    m_renderer = std::make_unique<DeviceRenderer>();
    m_window = std::make_shared<Window>(WIDTH, HEIGHT, "FaRT - " + scene + "@" + m_renderer->name());
    m_scene = std::make_unique<Scene>(scene);

    m_renderer->init(m_scene, m_window);

    SUCC("Finished initializing renderer (" + m_renderer->name() + ")");
}

void
App::run() {

    while(!m_window->shouldClose()) {
        // camera update
        if (m_window->isMouseLeftPressed())
            m_camera.rotate(m_window->getPrevMousePosition(), m_window->getMousePosition());
        if (m_window->isMouseRightPressed())
            m_camera.zoom(m_window->getDeltaMousePosition().y * 50.f);

        // render pass
        m_renderer->render(m_camera.eye(), m_camera.dir(), m_camera.up());

        glfwPollEvents();
    }
}

}
