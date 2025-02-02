#pragma once

#include <webgpu/webgpu.h>

#include "texture.h"
#include "pipeline.h"
#include "shader.h"
#include "buffer.h"

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
        std::unique_ptr<Shader> m_pathtracing_shader    { nullptr };
        std::unique_ptr<Pipeline> m_pathtracing_pipeline { nullptr };
        std::unique_ptr<Texture> m_accum_texture0       { nullptr };
        std::unique_ptr<Texture> m_accum_texture1       { nullptr };

        // WebGPU data resources
        std::unique_ptr<Buffer<float>> m_input_buffer   { nullptr };
        std::unique_ptr<Buffer<float>> m_output_buffer  { nullptr };
        std::unique_ptr<Buffer<float>> m_map_buffer     { nullptr };
        // WGPUBuffer m_input_buffer       { nullptr };
        // WGPUBuffer m_output_buffer      { nullptr };
        // WGPUBuffer m_map_buffer         { nullptr };
        // size_t m_buffersize             { 64 * sizeof(float) };

        // Private helper functions
        void initWebGPU();
        void initBuffers(); 
        void initTextures();
        // void initBindgroupLayout();
        // void initBindgroup();
        void initPipeline();
        void initBufferData();
        void renderpassPathtracer();

        // TODO: Temporary; remove later
        void renderpassReadOutputs();
        bool mappingdone = false;
};

}