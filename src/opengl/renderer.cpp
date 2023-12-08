#include "gldefs.h"
#include "renderer.h"
#include <cwchar>
#include <memory>
#include <stdexcept>

namespace fart {

void
OpenGlRenderer::init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) {
    m_scene = scene;
    m_window = window;

    initGl();
    initBVH();
    initBuffers();
    initShaders();
    initBindings();
}

void
OpenGlRenderer::initBVH() {
    m_bvh = std::make_unique<BVH>(m_scene->getMeshes()[0].geometries[0].vertices, m_scene->getMeshes()[0].geometries[0].indices);
}

void
OpenGlRenderer::initBuffers() {
    m_quad = std::make_unique<Buffer>(GL_ARRAY_BUFFER);
    m_vertices = std::make_unique<StorageBuffer>(0);
    m_indices = std::make_unique<StorageBuffer>(1);
    m_bvh_buffer = std::make_unique<StorageBuffer>(2);

    uint32_t index_offset = 0;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
    for (auto& mesh : m_scene->getMeshes()) {
        for (auto& geometry : mesh.geometries) {
            for (size_t i = 0; i < geometry.vertices.size(); i++) {
                vertices.emplace_back(geometry.vertices[i]);

                // TODO this is to obay the 16-bit alignment of SSBOs and should be optimized
                if (i % 3 == 2)
                    vertices.emplace_back(0);
            }
            for (auto& idx : geometry.indices)
                indices.emplace_back(idx+index_offset);

            index_offset = vertices.size() / 3;
        }
    }
    
    m_vertices->setData(vertices);
    m_indices->setData(indices);
    m_bvh_buffer->setData(m_bvh->getNodes());

    std::vector<float> quad {
        // first triangle
         1.f,  1.f, 0.0f,  // top right
         1.f, -1.f, 0.0f,  // bottom right
        -1.f,  1.f, 0.0f,  // top left 
        // second triangle
         1.f, -1.f, 0.0f,  // bottom right
        -1.f, -1.f, 0.0f,  // bottom left
        -1.f,  1.f, 0.0f   // top left
    };
    m_quad->setData(quad);
}

void
OpenGlRenderer::initShaders() {
    m_shader = std::make_unique<Shader>(
            "pathtracer.vert.glsl", 
            "pathtracer.frag.glsl"
            );
}

void
OpenGlRenderer::initBindings() {
    m_vertex_array = std::make_unique<VertexArray>();
    m_vertex_array->bind();
    m_quad->bind();
    m_vertices->bind();
    m_indices->bind();
    m_bvh_buffer->bind();
    m_vertex_array->addVertexAttribute(/*shader=*/*m_shader.get(), 
                                       /*attribute_name=*/"a_position", 
                                       /*size=*/3, 
                                       /*dtype=*/GL_FLOAT, 
                                       /*stride=*/3 * sizeof(float));
    m_vertex_array->unbind();
    m_quad->unbind();
    m_vertices->unbind();
    m_indices->unbind();
    m_bvh_buffer->unbind();
}

void
OpenGlRenderer::initGl() {
    glfwMakeContextCurrent(m_window->getGlfwWindow());
    glewExperimental = true;
    GLenum status = glewInit();
    if(status != GLEW_OK) {
        ERR("Failed to initialize GLEW");
    }

    glClearColor(0.12f, 0.1f, 0.1f, 1.f);
    glViewport(0, 0, m_window->getWidth(), m_window->getHeight());

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
OpenGlRenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up) {
    m_shader->use();
    //m_shader->setFloat4x4("u_vp", &vp[0][0]);
    auto viewport_size = m_window->getViewportSize();
    float aspect_ratio = (float)viewport_size.x / viewport_size.y;
    m_shader->setUInt2("u_viewport_size", glm::value_ptr(viewport_size));
    m_shader->setFloat("u_aspect_ratio", &aspect_ratio);
    m_shader->setFloat3("u_camera.eye", glm::value_ptr(eye));
    m_shader->setFloat3("u_camera.dir", glm::value_ptr(dir));
    m_shader->setFloat3("u_camera.up", glm::value_ptr(up));

    m_vertex_array->bind();
    m_vertices->bind();
    m_indices->bind();
    m_bvh_buffer->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDrawElements(GL_TRIANGLES, m_indices->getNElements(), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_vertex_array->unbind();
    m_vertices->unbind();
    m_indices->unbind();
    m_bvh_buffer->unbind();
    m_shader->unuse();

    glfwSwapBuffers(m_window->getGlfwWindow());
}

}
