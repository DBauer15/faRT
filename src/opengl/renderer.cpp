#include "gldefs.h"
#include "renderer.h"
#include <memory>
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
    initTextures();
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
    m_materials = std::make_unique<StorageBuffer>(3);

    m_vertices->setData(m_bvh->getVertices());
    m_indices->setData(m_bvh->getIndices());
    m_bvh_buffer->setData(m_bvh->getNodes().data(), m_bvh->getNodesUsed());
    m_materials->setData(m_scene->getMaterials());

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
OpenGlRenderer::initTextures() {
    for (auto& image : m_scene->getTextures()) {
        if (m_textures.size() >= 63) {
            WARN("Maximum supported number of textures (63) reached, skipping the rest");
            break;
        }
        GLenum format = image.getChannels() == 4 ? GL_RGBA : GL_RGB;
        GLenum src_type = GL_UNSIGNED_BYTE;
        Texture texture(image.getWidth(),
                        image.getHeight(),
                        GL_RGBA,
                        GL_RGBA,
                        src_type);
        texture.setData(image.getData(), 
                        GL_LINEAR_MIPMAP_NEAREST, 
                        GL_LINEAR,
                        GL_MIRRORED_REPEAT,
                        GL_MIRRORED_REPEAT);
        m_textures.push_back(std::move(texture));
    }
}

void
OpenGlRenderer::initShaders() {
    m_shader_pathtracer = std::make_unique<Shader>(
            "pathtracer.vert.glsl", 
            "pathtracer.frag.glsl"
            );

    m_shader_postprocess = std::make_unique<Shader>(
            "postprocess.vert.glsl", 
            "postprocess.frag.glsl"
            );
}

void
OpenGlRenderer::initBindings() {
    m_vertex_array_pathtracer = std::make_unique<VertexArray>();
    m_vertex_array_pathtracer->bind();
    m_quad->bind();
    m_vertices->bind();
    m_indices->bind();
    m_bvh_buffer->bind();
    m_vertex_array_pathtracer->addVertexAttribute(/*shader=*/*m_shader_pathtracer.get(), 
                                       /*attribute_name=*/"a_position", 
                                       /*size=*/3, 
                                       /*dtype=*/GL_FLOAT, 
                                       /*stride=*/3 * sizeof(float));
    m_vertex_array_pathtracer->unbind();
    m_quad->unbind();
    m_vertices->unbind();
    m_indices->unbind();
    m_bvh_buffer->unbind();

    m_vertex_array_postprocess = std::make_unique<VertexArray>();
    m_vertex_array_postprocess->bind();
    m_quad->bind();
    m_vertex_array_postprocess->addVertexAttribute(/*shader=*/*m_shader_postprocess.get(), 
                                       /*attribute_name=*/"a_position", 
                                       /*size=*/3, 
                                       /*dtype=*/GL_FLOAT, 
                                       /*stride=*/3 * sizeof(float));
    m_vertex_array_postprocess->unbind();
    m_quad->unbind();
}

void
OpenGlRenderer::initGl() {
    glfwMakeContextCurrent(m_window->getGlfwWindow());
    glfwSwapInterval(0);
    gladLoadGL();

    glViewport(0, 0, m_window->getWidth(), m_window->getHeight());
}

bool
OpenGlRenderer::shouldClear(const glm::vec3& eye, const glm::vec3& dir, const glm::vec3& up) {
    bool clear = glm::any(glm::epsilonNotEqual(eye, m_prev_eye, 0.00001f)) ||
                 glm::any(glm::epsilonNotEqual(dir, m_prev_dir, 0.00001f)) ||
                 glm::any(glm::epsilonNotEqual(up, m_prev_up, 0.00001f));

    m_prev_eye = eye;
    m_prev_dir = dir;
    m_prev_up = up;

    return clear;
}

void
OpenGlRenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) {
    auto t_start = std::chrono::high_resolution_clock::now();
    float scene_scale = m_scene->getSceneScale();
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


    { // Pathtracing renderpass
        m_framebuffer0->bind();
        m_shader_pathtracer->use();
        m_shader_pathtracer->setUInt("u_frame_no", &m_frame_no);
        m_shader_pathtracer->setFloat("u_scene_scale", &scene_scale);
        m_shader_pathtracer->setUInt2("u_viewport_size", glm::value_ptr(viewport_size));
        m_shader_pathtracer->setFloat("u_aspect_ratio", &aspect_ratio);
        m_shader_pathtracer->setFloat3("u_camera.eye", glm::value_ptr(eye));
        m_shader_pathtracer->setFloat3("u_camera.dir", glm::value_ptr(dir));
        m_shader_pathtracer->setFloat3("u_camera.up", glm::value_ptr(up));
        int u_frag_color_accum_pos = 63;
        m_accum_texture1->activate(GL_TEXTURE0 + u_frag_color_accum_pos);
        m_accum_texture1->bind();
        m_shader_pathtracer->setInt("u_frag_color_accum", &u_frag_color_accum_pos);
        for (int i = 0; i < m_textures.size(); i++) {
            m_textures[i].activate(GL_TEXTURE0 + i);
            m_textures[i].bind();
            m_shader_pathtracer->setInt("u_textures[" + std::to_string(i) + "]", &i);
        }

        m_vertex_array_pathtracer->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_vertex_array_pathtracer->unbind();

        m_accum_texture1->unbind();
        m_shader_pathtracer->unuse();
        m_framebuffer0->unbind();
    }

    { // Postprocessing renderpass
        m_shader_postprocess->use();
        int u_frag_color_accum_pos = 63;
        m_accum_texture0->activate(GL_TEXTURE0 + u_frag_color_accum_pos);
        m_accum_texture0->bind();
        m_shader_postprocess->setInt("u_frag_color_accum", &u_frag_color_accum_pos);

        m_vertex_array_postprocess->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_vertex_array_postprocess->unbind();

        m_shader_postprocess->unuse();
    }

    glfwSwapBuffers(m_window->getGlfwWindow());
    m_framebuffer0.swap(m_framebuffer1);
    m_accum_texture0.swap(m_accum_texture1);

    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;
    m_frame_no += 1;
}

}
