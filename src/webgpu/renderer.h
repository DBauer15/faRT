#pragma once

#include <webgpu/webgpu.h>

#include "texture.h"
#include "common/renderer.h"
#include "common/window.h"

namespace fart {

struct WebGPURenderer : Renderer {
    public:
        ~WebGPURenderer();
        void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) override;
        void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) override;
        virtual std::string name() override {
            return "WebGPU Renderer";
        }

        virtual size_t preferredVertexAlignment() override {
            return 8;
        }

    private:
        std::shared_ptr<Scene>  m_scene;
        std::shared_ptr<Window> m_window;

        // Helper functions for WebGPU
        WGPUAdapter requestAdapterSync(WGPUInstance instance);
        WGPUDevice  requestDeviceSync(WGPUAdapter adapter);
        void        resizeSurface(WGPUSurface surface, WGPUAdapter adapter, WGPUDevice device, uint32_t width, uint32_t height);

        // Objects related to WebGPU
        WGPUSurface m_surface;
        WGPUInstance m_instance;
        WGPUAdapter m_adapter;
        WGPUDevice m_device;
        WGPUQueue m_queue;

        // Pathtracing specific WebGPU objects
        std::unique_ptr<Texture> m_accum_texture0       { nullptr };
        std::unique_ptr<Texture> m_accum_texture1       { nullptr };

        // Private helper functions
        void initWebGPU();
        void renderpassPathtracer();
        void renderpassPostprocess();
};

}