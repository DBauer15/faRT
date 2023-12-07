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

namespace fart {

struct MetalRenderer : Renderer {

    public:
        ~MetalRenderer();

        virtual void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) override; 

        virtual void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up) override;

        virtual std::string name() override {
            return "Metal Renderer";
        }

    private:
        std::shared_ptr<Scene> m_scene { nullptr };

        MTL::Device*                m_device                { nullptr };
        MTL::CommandQueue*          m_command_queue         { nullptr };
        MTL::RenderPipelineState*   m_render_pipeline_state { nullptr };
        CA::MetalLayer*             m_layer                 { nullptr };
        CA::MetalDrawable*          m_drawable              { nullptr };

        void addLayerToWindow(GLFWwindow* window, CA::MetalLayer* layer);

        void sendRenderCommand();
        void encodeRenderCommand(MTL::RenderCommandEncoder* renderCommandEncoder);

        MTL::Buffer*                m_triangle_buffer       { nullptr };
        void createTriangle();


};


}
