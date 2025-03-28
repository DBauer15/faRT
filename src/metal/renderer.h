#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <memory>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "common/defs.h"
#include "common/renderer.h"

#include "mesh.h"
#include "texture.h"

namespace fart {

// TODO: Fix alignment and not waste space
struct MetalRendererUniforms {
    uint32_t frame_number;
    float scene_scale;
    float aspect_ratio;
    float filler;
    glm::vec4 eye;
    glm::vec4 dir;
    glm::vec4 up;
    glm::vec4 viewport_size;
};

struct MetalRenderer : Renderer {

    public:
        ~MetalRenderer();

        virtual void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) override; 

        virtual void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) override;

        virtual std::string name() override {
            return "Metal Renderer";
        }

        virtual size_t preferredVertexAlignment() override {
            return 4;
        }

    private:
        glm::vec3 m_prev_eye, m_prev_dir, m_prev_up;

        std::shared_ptr<Window> m_window { nullptr };
        std::shared_ptr<Scene> m_scene { nullptr };

        // Objects and functions needed to interface between Metal and GLFW
        CA::MetalLayer* m_layer                             { nullptr };
        void addLayerToWindow(GLFWwindow* window, CA::MetalLayer* layer);
        void resizeLayer(CA::MetalLayer* layer, uint32_t width, uint32_t height);

        // Common Metal objects
        MTL::Device* m_device                               { nullptr };
        MTL::CommandQueue* m_command_queue                  { nullptr };
        MTL::Library* m_library                             { nullptr };

        // Pathtracing specific Metal objects
        MTL::ComputePipelineState* m_pathtracing_pipeline_state         { nullptr };
        MTL::RenderPipelineState* m_postprocess_pipeline_state          { nullptr };
        std::unique_ptr<Texture> m_accum_texture0                       { nullptr };
        std::unique_ptr<Texture> m_accum_texture1                       { nullptr };
        MTL::IntersectionFunctionTable* m_intersection_function_table   { nullptr };
        MTL::AccelerationStructure* m_tlas                              { nullptr };
        std::vector<MTL::AccelerationStructure*> m_blas_list;

        // Metal data resources
        std::vector<MetalObject> m_objects;
        MTL::Buffer* m_instances             { nullptr };
        MTL::Buffer* m_uniforms              { nullptr };
        MTL::Buffer* m_resources             { nullptr };
        MTL::Buffer* m_materials             { nullptr };
        MTL::Buffer* m_textures              { nullptr };
        std::vector<Texture> m_texture_list;
        uint32_t m_resources_geometry_stride;

        // Private helper functions
        void initMetal();
        void initFrameBuffer();
        void initBuffers();
        void initIntersectionFunctionTable();
        void initAccelerationStructure();
        void initPipeline();
        void renderpassPathtracer(MTL::CommandBuffer* command_buffer);
        void renderpassPostprocess(MTL::CommandBuffer* command_buffer);
        bool shouldClear(const glm::vec3& eye, const glm::vec3& dir, const glm::vec3& up);

        MTL::AccelerationStructure* createAccelerationStructureWithDescriptor(MTL::AccelerationStructureDescriptor* descriptor);
};


}
