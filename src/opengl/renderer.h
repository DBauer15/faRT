#pragma once

#include "buffer.h"
#include "opengl/vertex_array.h"
#include "shader.h"
#include "common/renderer.h"
#include "common/window.h"

namespace fart {

struct OpenGlRenderer : Renderer {
    public:
        void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) override;
        void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up) override;
        virtual std::string name() override {
            return "OpenGL Renderer";
        }

    private:
        std::shared_ptr<Scene> m_scene;
        std::shared_ptr<Window> m_window;

        std::unique_ptr<Buffer> m_vertices;
        std::unique_ptr<Buffer> m_normals;
        std::unique_ptr<Buffer> m_indices;

        std::unique_ptr<VertexArray> m_vertex_array;
        std::unique_ptr<Shader> m_shader;

        void initBuffers();
        void initShaders();
        void initBindings();
        void initGl();
};

}
