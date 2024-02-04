#pragma once

#include "buffer.h"
#include "bvh.h"
#include "tlas.h"
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
        glm::vec3 m_prev_eye, m_prev_dir, m_prev_up;

        std::shared_ptr<Scene> m_scene;
        std::shared_ptr<Window> m_window;
        std::vector<BVHNode> m_blas_list;
        std::shared_ptr<TLAS> m_tlas;
        std::vector<AligendVertex> m_vertices_contiguous;
        std::vector<uint32_t> m_indices_contiguous;

        std::unique_ptr<Buffer> m_quad;

        std::unique_ptr<StorageBuffer> m_blas_buffer;
        std::unique_ptr<StorageBuffer> m_tlas_buffer;
        std::unique_ptr<StorageBuffer> m_vertices;
        std::unique_ptr<StorageBuffer> m_indices;
        std::unique_ptr<StorageBuffer> m_materials;
        std::vector<Texture> m_textures;

        std::unique_ptr<VertexArray> m_vertex_array_pathtracer;
        std::unique_ptr<Shader> m_shader_pathtracer;
        std::unique_ptr<VertexArray> m_vertex_array_postprocess;
        std::unique_ptr<Shader> m_shader_postprocess;

        std::unique_ptr<FrameBuffer> m_framebuffer0;
        std::unique_ptr<FrameBuffer> m_framebuffer1;
        std::unique_ptr<Texture> m_accum_texture0;
        std::unique_ptr<Texture> m_accum_texture1;

        void initAccelerationStructures();
        void initFrameBuffer();
        void initBuffers();
        void initTextures();
        void initShaders();
        void initBindings();
        void initGl();
        bool shouldClear(const glm::vec3& eye, const glm::vec3& dir, const glm::vec3& up);
};

}
