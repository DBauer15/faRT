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
    initBuffers();
    initShaders();
    initBindings();
}

void
OpenGlRenderer::initBuffers() {
    m_vertices = std::make_unique<Buffer>(GL_ARRAY_BUFFER);
    m_normals = std::make_unique<Buffer>(GL_ARRAY_BUFFER);
    m_indices = std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER);

    uint32_t index_offset = 0;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
    for (auto& mesh : m_scene->getMeshes()) {
        for (auto& geometry : mesh.geometries) {
            for (auto& vert : geometry.vertices)
                vertices.emplace_back(vert);
            for (auto& norm : geometry.normals)
                normals.emplace_back(norm);
            for (auto& idx : geometry.indices)
                indices.emplace_back(idx+index_offset);

            index_offset = vertices.size() / 3;
        }
    }
    
    m_vertices->setData(vertices);
    m_normals->setData(normals);
    m_indices->setData(indices);
}

void
OpenGlRenderer::initShaders() {
    m_shader = std::make_unique<Shader>(
            "triangle.vert.glsl", 
            "triangle.frag.glsl"
            );
}

void
OpenGlRenderer::initBindings() {
    m_vertex_array = std::make_unique<VertexArray>();
    m_vertex_array->bind();
    m_vertices->bind();
    m_indices->bind();
    m_vertex_array->addVertexAttribute(/*shader=*/*m_shader.get(), 
                                       /*attribute_name=*/"a_position", 
                                       /*size=*/3, 
                                       /*dtype=*/GL_FLOAT, 
                                       /*stride=*/3 * sizeof(float));
    m_normals->bind();
    m_vertex_array->addVertexAttribute(/*shader=*/*m_shader.get(), 
                                       /*attribute_name=*/"a_normal", 
                                       /*size=*/3, 
                                       /*dtype=*/GL_FLOAT, 
                                       /*stride=*/3 * sizeof(float));
    m_vertex_array->unbind();
    m_indices->unbind();
    m_normals->unbind();
}

void
OpenGlRenderer::initGl() {
    glfwMakeContextCurrent(m_window->getGlfwWindow());
    glewExperimental = true;
    GLenum status = glewInit();
    if(status != GLEW_OK) {
        ERR("Failed to initialize GLEW");
    }

    glClearColor(0.f, 0.2f, 1.f, 1.f);
    glViewport(0, 0, m_window->getWidth(), m_window->getHeight());

    glEnable(GL_DEPTH_TEST);
}

void
OpenGlRenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up) {
    // make vp matrix
    glm::mat4 perspective = glm::perspective(glm::radians(45.f), (float)m_window->getWidth() / m_window->getHeight(), 0.1f, 1000.f);
    glm::mat4 view = glm::lookAt(eye, {0.f, 0.f, 0.f}, up);
    glm::mat4 vp = perspective * view;

    m_shader->use();
    m_shader->setFloat4x4("u_vp", &vp[0][0]);

    m_vertex_array->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, m_indices->getNElements(), GL_UNSIGNED_INT, 0);

    m_vertex_array->unbind();
    m_shader->unuse();

    glfwSwapBuffers(m_window->getGlfwWindow());
}

}
