#include "gldefs.h"
#include "renderer.h"
#include <cwchar>
#include <memory>
#include <stdexcept>
#include <chrono>

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
    m_bvh = std::make_unique<BVH>(m_scene->getMeshes()[0].geometries);
}

void
OpenGlRenderer::initBuffers() {
    m_quad = std::make_unique<Buffer>(GL_ARRAY_BUFFER);
    m_vertices = std::make_unique<StorageBuffer>(0);
    m_indices = std::make_unique<StorageBuffer>(1);
    m_bvh_buffer = std::make_unique<StorageBuffer>(2);

    std::vector<float> vertices;
    uint32_t n_inserted = 0;
    for (auto& vertex : m_bvh->getVertices()) {
        vertices.emplace_back(vertex);
        if (++n_inserted % 3 == 0) {
            vertices.emplace_back(0);
        }
    }
    
    m_vertices->setData(vertices);
    m_indices->setData(m_bvh->getIndices());
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
    glfwSwapInterval(0);
    gladLoadGL();
    //glewExperimental = true;
    //GLenum status = glewInit();
    //if(status != GLEW_OK) {
        //ERR("Failed to initialize GLEW");
    //}

    glClearColor(0.12f, 0.1f, 0.1f, 1.f);
    glViewport(0, 0, m_window->getWidth(), m_window->getHeight());

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
OpenGlRenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) {
    auto t_start = std::chrono::high_resolution_clock::now();

    m_shader->use();
    auto viewport_size = m_window->getViewportSize();
    float aspect_ratio = (float)viewport_size.x / viewport_size.y;
    m_shader->setUInt("u_frame_no", &m_frame_no);
    m_shader->setUInt2("u_viewport_size", glm::value_ptr(viewport_size));
    m_shader->setFloat("u_aspect_ratio", &aspect_ratio);
    m_shader->setFloat3("u_camera.eye", glm::value_ptr(eye));
    m_shader->setFloat3("u_camera.dir", glm::value_ptr(dir));
    m_shader->setFloat3("u_camera.up", glm::value_ptr(up));

    m_vertex_array->bind();
    m_vertices->bind();
    m_indices->bind();
    m_bvh_buffer->bind();

    glViewport(0, 0, viewport_size.x, viewport_size.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_vertex_array->unbind();
    m_vertices->unbind();
    m_indices->unbind();
    m_bvh_buffer->unbind();
    m_shader->unuse();

    glfwSwapBuffers(m_window->getGlfwWindow());

    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;
    m_frame_no += 1;
}

}
