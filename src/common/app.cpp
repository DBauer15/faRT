#include "app.h"
#include "metal/renderer.h"
#include "scene.h"
#include <memory>
#include <GLFW/glfw3.h>

#include <iostream>
namespace fart {

App::App(std::string scene) {
    m_scene = std::make_unique<Scene>(scene);
    m_renderer.reset(new MetalRenderer());

    m_renderer->init(*m_scene.get(), m_window);

    SUCC("Finished initializing renderer (" + m_renderer->name() + ")");
}

void
App::run() {

    while(!m_window.shouldClose()) {
        m_renderer->render();
        glfwPollEvents();
    }
}

}
