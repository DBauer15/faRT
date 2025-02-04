#pragma once

// Renderer Selection
#if OPENGL_RENDERER
#include "opengl/renderer.h"
using DeviceRenderer = fart::OpenGlRenderer;
#elif METAL_RENDERER
#include "metal/renderer.h"
using DeviceRenderer = fart::MetalRenderer;
#elif WEBGPU_RENDERER
#include "webgpu/renderer.h"
using DeviceRenderer = fart::WebGPURenderer;
#endif

#include "camera.h"
#include "renderer.h"
#include "defs.h"
#include "window.h"
#include <stage.h>
#include <memory>

using namespace stage;

namespace fart {

struct App {
    
    public:
        static constexpr uint32_t WIDTH = 1280;
        static constexpr uint32_t HEIGHT = 720;

        App(std::string scene);
        ~App() = default;

        void run();

    private:
        void processFrame();
        glm::vec3 keyboardInputToMovementVector();

        bool m_camera_mode_changed { false };
        float m_fps { -1.f };
        float m_fps_ema { -1.f };
        long long m_frame_count { 0 };
        
        std::shared_ptr<Camera> m_camera {nullptr};
        std::shared_ptr<Window> m_window {nullptr};
        std::shared_ptr<Scene> m_scene {nullptr};
        std::unique_ptr<Renderer> m_renderer {nullptr};
};

}
