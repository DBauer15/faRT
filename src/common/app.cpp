#include "app.h"
#include <memory>
#include <string>

namespace fart {

App::App(std::string scene) {
    m_renderer = std::make_unique<DeviceRenderer>();
    m_scene = std::make_shared<Scene>(scene);
    m_camera = std::make_shared<FirstPersonCamera>(
            glm::vec3( 0.f, 0.f, -m_scene->getSceneScale() ), 
            glm::vec3( 0.f, 0.f, 0.f ), 
            glm::vec3( 0.f, 1.f, 0.f ));
    if (m_scene->getCamera())  
        m_camera = std::make_shared<FirstPersonCamera>(m_scene->getCamera()->position, m_scene->getCamera()->lookat, m_scene->getCamera()->up);
    m_window = std::make_shared<Window>(WIDTH, HEIGHT, "FaRT - " + scene + " @ " + m_renderer->name());

    m_renderer->init(m_scene, m_window);

    SUCC("Finished initializing renderer (" + m_renderer->name() + ")");
}

void
App::run() {

    while(!m_window->shouldClose()) {
        // update window
        m_window->update();

        // process input
        if (m_window->isWindowFocused()) {
            // camera update
            if (m_window->isMouseLeftPressed())
                m_camera->rotate(m_window->getPrevMousePosition(), m_window->getMousePosition());
            if (m_window->isMouseRightPressed())
                m_camera->zoom(m_window->getDeltaMousePosition().y * m_scene->getSceneScale());
            if (m_window->isMouseMiddlePressed())
                m_camera->pan(m_window->getDeltaMousePosition());
            m_camera->move(keyboardInputToMovementVector() * m_scene->getSceneScale());

            // camera mode
            if (!m_camera_mode_changed && m_window->isKeyPressed(GLFW_KEY_C)) {
                m_camera_mode_changed = true;
                if (typeid(*m_camera) == typeid(ArcballCamera))
                    m_camera = std::make_unique<FirstPersonCamera>(*m_camera);
                else
                    m_camera = std::make_unique<ArcballCamera>(*m_camera);
            }
            if (!m_window->isKeyPressed(GLFW_KEY_C)) {
                m_camera_mode_changed = false;
            }
        }

        // render pass
        RenderStats render_stats;
        m_renderer->render(m_camera->eye(), m_camera->dir(), m_camera->up(), render_stats);

        m_fps = (1000.f / render_stats.frame_time_ms);
        if (m_fps_ema < 0.f) m_fps_ema = m_fps; 
        m_fps_ema = 0.05f * m_fps + 0.95f * m_fps_ema;
        if (m_fps_ema / ( m_frame_count + 1 ) < 5.f) {
            m_window->setWindowTitle("FaRT - " + m_renderer->name() + " @ " + std::to_string(int(m_fps_ema)) + " fps");
            m_frame_count = 0;
        }

        glfwPollEvents();

        m_frame_count += 1;
    }
}

glm::vec3
App::keyboardInputToMovementVector() {
    glm::vec3 movement = glm::vec3(0);
    float s = .1f * (1.f / m_fps);
    movement += glm::vec3(s,0,0) * float(m_window->isKeyPressed(GLFW_KEY_A));
    movement += glm::vec3(-s,0,0) * float(m_window->isKeyPressed(GLFW_KEY_D));
    movement += glm::vec3(0,0,-s) * float(m_window->isKeyPressed(GLFW_KEY_W));
    movement += glm::vec3(0,0,s) * float(m_window->isKeyPressed(GLFW_KEY_S));
    movement += glm::vec3(0,-s,0) * float(m_window->isKeyPressed(GLFW_KEY_E));
    movement += glm::vec3(0,s,0) * float(m_window->isKeyPressed(GLFW_KEY_Q));

    if (m_window->isKeyPressed(GLFW_KEY_SPACE))
        movement *= 10.f;

    return movement;
}

}
