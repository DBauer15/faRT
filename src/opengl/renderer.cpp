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
    initFrameBuffer();
    initBuffers();
    initShaders();
    initBindings();
}

void
OpenGlRenderer::initBVH() {
    m_bvh = std::make_unique<BVH>(m_scene->getMeshes()[0].geometries);
}

void
OpenGlRenderer::initFrameBuffer() {
    m_framebuffer0 = std::make_unique<FrameBuffer>();
    m_framebuffer1 = std::make_unique<FrameBuffer>();
    m_accum_texture0 = std::make_unique<Texture>(m_window->getWidth(), m_window->getHeight());
    m_accum_texture1 = std::make_unique<Texture>(m_window->getWidth(), m_window->getHeight());

    m_framebuffer0->addAttachment(*m_accum_texture0, GL_COLOR_ATTACHMENT0);
    m_framebuffer1->addAttachment(*m_accum_texture1, GL_COLOR_ATTACHMENT0);

    if (!m_framebuffer0->isComplete()) {
        ERR("Framebuffer 0 was not built correctly.");
    }
    if (!m_framebuffer1->isComplete()) {
        ERR("Framebuffer 1 was not built correctly.");
    }
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
    m_shader_pathtracer = std::make_unique<Shader>(
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
    m_vertex_array->addVertexAttribute(/*shader=*/*m_shader_pathtracer.get(), 
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

    glViewport(0, 0, m_window->getWidth(), m_window->getHeight());
}

void
OpenGlRenderer::blit() {
    auto viewport_size = m_window->getViewportSize();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer0->getFrameBuffer());
    glBlitFramebuffer(0, 0, viewport_size.x, viewport_size.y, 
                      0, 0, viewport_size.x, viewport_size.y,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

bool
OpenGlRenderer::shouldClear(const glm::vec3& eye, const glm::vec3& dir, const glm::vec3& up) {
    bool clear = glm::any(glm::epsilonNotEqual(eye, m_prev_eye, 0.00001f)) |
                 glm::any(glm::epsilonNotEqual(dir, m_prev_dir, 0.00001f)) |
                 glm::any(glm::epsilonNotEqual(up, m_prev_up, 0.00001f));

    m_prev_eye = eye;
    m_prev_dir = dir;
    m_prev_up = up;

    return clear;
}

void
OpenGlRenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) {
    auto t_start = std::chrono::high_resolution_clock::now();
    auto viewport_size = m_window->getViewportSize();
    float aspect_ratio = (float)viewport_size.x / viewport_size.y;
    glViewport(0, 0, viewport_size.x, viewport_size.y);

    m_accum_texture0->resize(viewport_size.x, viewport_size.y);
    m_accum_texture1->resize(viewport_size.x, viewport_size.y);

    if (shouldClear(eye, dir, up)) {
        m_frame_no = 0;
        m_accum_texture0->clear();
        m_accum_texture1->clear();
    }

    m_framebuffer0->bind();
    m_shader_pathtracer->use();
    m_shader_pathtracer->setUInt("u_frame_no", &m_frame_no);
    m_shader_pathtracer->setUInt2("u_viewport_size", glm::value_ptr(viewport_size));
    m_shader_pathtracer->setFloat("u_aspect_ratio", &aspect_ratio);
    m_shader_pathtracer->setFloat3("u_camera.eye", glm::value_ptr(eye));
    m_shader_pathtracer->setFloat3("u_camera.dir", glm::value_ptr(dir));
    m_shader_pathtracer->setFloat3("u_camera.up", glm::value_ptr(up));
    m_shader_pathtracer->setUInt("u_frag_color_accum", 0);
    m_accum_texture1->activate(GL_TEXTURE0);
    m_accum_texture1->bind();

    m_vertex_array->bind();
    m_vertices->bind();
    m_indices->bind();
    m_bvh_buffer->bind();

    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_bvh_buffer->unbind();
    m_vertex_array->unbind();
    m_vertices->unbind();
    m_indices->unbind();
    m_accum_texture1->unbind();
    m_shader_pathtracer->unuse();
    m_framebuffer0->unbind();

    blit();
    glfwSwapBuffers(m_window->getGlfwWindow());
    m_framebuffer0.swap(m_framebuffer1);
    m_accum_texture0.swap(m_accum_texture1);

    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;
    m_frame_no += 1;
}

}
