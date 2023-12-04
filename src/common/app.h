#pragma once

// Renderer Selection
#if OPENGL_RENDERER
#include "opengl/renderer.h"
using DeviceRenderer = fart::OpenGlRenderer;
#elif METAL_RENDERER
#include "metal/renderer.h"
using DeviceRenderer = fart::MetalRenderer;
#endif

#include "camera.h"
#include "renderer.h"
#include "defs.h"
#include "window.h"
#include "scene.h"
#include <memory>

namespace fart {

struct App {
    
    public:
        static constexpr uint32_t WIDTH = 1920;
        static constexpr uint32_t HEIGHT = 1080;

        App(std::string scene);
        ~App() = default;

        void run();

    private:
        Camera m_camera{
            { 0.f, 0.f, -1.f },    // eye
            { 0.f, 0.f, 0.f },      // center
            { 0.f, 1.f, 0.f }       // up
        };

        std::shared_ptr<Window> m_window {nullptr};
        std::shared_ptr<Scene> m_scene {nullptr};
        std::unique_ptr<Renderer> m_renderer {nullptr};
};

}
