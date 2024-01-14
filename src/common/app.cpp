#include "app.h"
#include "scene.h"
#include <memory>
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
        }

        // render pass
        RenderStats render_stats;
        m_renderer->render(m_camera->eye(), m_camera->dir(), m_camera->up(), render_stats);
        if (m_fps_ema < 0.f) m_fps_ema = (1000.f / render_stats.frame_time_ms); 
        m_fps_ema = 0.01f * (1000.f / render_stats.frame_time_ms) + 0.99f * m_fps_ema;
        if (m_frame_count % 100 == 0) m_window->setWindowTitle("FaRT - " + m_renderer->name() + " @ " + std::to_string(int(std::floorf(m_fps_ema))) + " fps");

        glfwPollEvents();

        m_frame_count += 1;
    }
}

}
