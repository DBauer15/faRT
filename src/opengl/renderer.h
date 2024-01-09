#pragma once

#include "buffer.h"
#include "bvh.h"
#include "framebuffer.h"
#include "vertex_array.h"
#include "shader.h"
#include "texture.h"
#include "common/renderer.h"
#include "common/window.h"

namespace fart {

struct OpenGlRenderer : Renderer {
    public:
        void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) override;
        void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) override;
        virtual std::string name() override {
            return "OpenGL Renderer";
        }

    private:
        uint32_t m_frame_no { 0 };
        std::shared_ptr<Scene> m_scene;
        std::shared_ptr<Window> m_window;
        std::shared_ptr<BVH> m_bvh;

        std::unique_ptr<Buffer> m_quad;

        std::unique_ptr<StorageBuffer> m_bvh_buffer;
        std::unique_ptr<StorageBuffer> m_vertices;
        std::unique_ptr<StorageBuffer> m_indices;

        std::unique_ptr<VertexArray> m_vertex_array;
        std::unique_ptr<Shader> m_shader_pathtracer;
        std::unique_ptr<Shader> m_shader_accumulator;

        std::unique_ptr<FrameBuffer> m_framebuffer;
        std::unique_ptr<Texture> m_single_pass_texture;
        std::unique_ptr<Texture> m_accum_texture;

        void initBVH();
        void initFrameBuffer();
        void initBuffers();
        void initShaders();
        void initBindings();
        void initGl();
        void blit();
};

}
