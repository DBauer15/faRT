#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <glm/glm.hpp>
#include <string>
#include "renderer.h"

namespace fart {

MetalRenderer::~MetalRenderer() {
    if (m_device)
        m_device->release();
    if (m_command_queue)
        m_command_queue->release();
    if (m_render_pipeline_state)
        m_render_pipeline_state->release();
}

void
MetalRenderer::init(Scene& scene, Window& window) {
    m_scene.reset(&scene);
    m_device = MTL::CreateSystemDefaultDevice();
    m_command_queue = m_device->newCommandQueue();

    m_layer = CA::MetalLayer::layer();
    m_layer ->setDevice(m_device);
    m_layer->setFramebufferOnly(true);
    m_layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    addLayerToWindow(window.getGlfwWindow(), m_layer);

    MTL::Library* library = m_device->newDefaultLibrary();
    MTL::Function* vertexFunction = library->newFunction(NS::String::string("vertexShader", NS::StringEncoding::ASCIIStringEncoding));
    MTL::Function* fragmentFunction = library->newFunction(NS::String::string("fragmentShader", NS::StringEncoding::ASCIIStringEncoding));

    MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineDescriptor->setLabel(NS::String::string("Simple Pipeline", NS::StringEncoding::UTF8StringEncoding));
    pipelineDescriptor->setVertexFunction(vertexFunction);
    pipelineDescriptor->setFragmentFunction(fragmentFunction);
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat((MTL::PixelFormat)m_layer->pixelFormat());

    NS::Error* error;
    m_render_pipeline_state = m_device->newRenderPipelineState(pipelineDescriptor, &error);

    if (error)
        ERR("Error compiling render pipeline");

    pipelineDescriptor->release();

    createTriangle();
}

void
MetalRenderer::render() {
    m_drawable = m_layer->nextDrawable();
    sendRenderCommand();
    m_drawable->release();
}

void
MetalRenderer::sendRenderCommand() {

    MTL::CommandBuffer* commandBuffer = m_command_queue->commandBuffer();

    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cad = renderPassDescriptor->colorAttachments()->object(0);

    cad->setTexture(m_drawable->texture());
    cad->setLoadAction(MTL::LoadActionClear);
    cad->setClearColor(MTL::ClearColor(1.0f, 0.0f, 0.0f, 1.0f));
    cad->setStoreAction(MTL::StoreActionStore);

    MTL::RenderCommandEncoder* renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
    encodeRenderCommand(renderCommandEncoder);
    renderCommandEncoder->endEncoding();

    commandBuffer->presentDrawable(m_drawable);
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();

    renderPassDescriptor->release();
}

void
MetalRenderer::encodeRenderCommand(MTL::RenderCommandEncoder* renderCommandEncoder) {
    renderCommandEncoder->setRenderPipelineState(m_render_pipeline_state);
    renderCommandEncoder->setVertexBuffer(m_triangle_buffer, 0, 0);

    MTL::PrimitiveType triangle = MTL::PrimitiveType::PrimitiveTypeTriangle;
    NS::UInteger start = 0;
    NS::UInteger count = 3;

    renderCommandEncoder->drawPrimitives(triangle, start, count);
}


void
MetalRenderer::createTriangle() {

    // Metal's float3 has a 16byte alignment, so we have to pad our values to 4x4bytes = 4 floats
    // Alternatively, we can use Metal's packed types to match GLM-styles alignment
    std::vector<float> triangleVertices {
        -1.f, -1.f, 0.f, 0.f,
        1.f, -1.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f
    };

    m_triangle_buffer = m_device->newBuffer(triangleVertices.data(),
                                            3 * 4 * sizeof(float),
                                            MTL::ResourceStorageModeShared);
}

}
