#pragma once

#include <webgpu/webgpu.h>

#include "texture.h"
#include "pipeline.h"
#include "shader.h"
#include "buffer.h"

#include "common/renderer.h"
#include "common/window.h"

namespace fart {

struct WebGPURendererUniforms {
    alignas(4) uint32_t frame_number;
    alignas(4) float scene_scale;
    alignas(4) float aspect_ratio;
    alignas(16) glm::vec4 eye;
    alignas(16) glm::vec4 dir;
    alignas(16) glm::vec4 up;
    alignas(16) glm::u32vec4 viewport_size;
};

struct WebGPURenderer : Renderer {
    public:
        ~WebGPURenderer();
        void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) override;
        void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) override;
        virtual std::string name() override {
            return "WebGPU Renderer";
        }

        virtual size_t preferredVertexAlignment() override {
            return 16;
        }

    private:
        uint32_t m_frame_no { 0 };
        std::shared_ptr<Scene>  m_scene;
        std::shared_ptr<Window> m_window;

        // Helper functions for WebGPU
        WGPUAdapter requestAdapterSync(WGPUInstance instance);
        WGPUDevice  requestDeviceSync(WGPUAdapter adapter);
        size_t      m_prev_window_width     { 0 };
        size_t      m_prev_window_height    { 0 };
        void        resize(WGPUSurface surface, WGPUAdapter adapter, WGPUDevice device, uint32_t width, uint32_t height);

        // Objects related to WebGPU
        WGPUSurface m_surface;
        WGPUInstance m_instance;
        WGPUAdapter m_adapter;
        WGPUDevice m_device;
        WGPUQueue m_queue;

        // Pathtracing specific WebGPU objects
        std::unique_ptr<Shader> m_pathtracing_shader    { nullptr };
        std::unique_ptr<ComputePipeline> m_pathtracing_pipeline { nullptr };
        std::unique_ptr<Shader> m_postprocessing_shader { nullptr };
        std::unique_ptr<RenderPipeline> m_postprocessing_pipeline { nullptr };
        std::unique_ptr<Texture> m_accum_texture0       { nullptr };
        std::unique_ptr<Texture> m_accum_texture1       { nullptr };

        // WebGPU data resources
        WebGPURendererUniforms         m_uniforms       { };
        std::unique_ptr<Buffer<WebGPURendererUniforms>> 
                                       m_uniforms_buffer{ nullptr };
        std::unique_ptr<Buffer<float>> m_fullscreen_quad_buffer { nullptr };

        // Private helper functions
        void initWebGPU();
        void initBuffers(); 
        void initTextures();
        void initPipeline();
        void initBufferData();
        void renderpassPathtracer();
        void renderpassPostprocess();

};

}
