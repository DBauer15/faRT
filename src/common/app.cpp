#include "app.h"
#include "scene.h"
#include <memory>
#include <GLFW/glfw3.h>

#include <iostream>
namespace fart {

App::App(std::string scene) {
    m_scene = std::make_unique<Scene>(scene);
}

void
App::run() {

    while(!m_window.shouldClose()) {
        glfwPollEvents();
    }
}

}
