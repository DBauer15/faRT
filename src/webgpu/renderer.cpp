#include "renderer.h"
#include <glfw3webgpu.h>

#include <embedded_shaders.h>

namespace fart {

WebGPURenderer::~WebGPURenderer()
{
    if (m_instance) {
        wgpuInstanceRelease(m_instance);
    }
    if (m_adapter) {
        wgpuAdapterRelease(m_adapter);
    }
    if (m_device) {
        wgpuDeviceRelease(m_device);
    }
    if (m_queue) {
        wgpuQueueRelease(m_queue);
    }
    if (m_surface) {
        wgpuSurfaceUnconfigure(m_surface);
        wgpuSurfaceRelease(m_surface);
    }
}

void 
WebGPURenderer::init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window)
{
    m_scene = scene;
    m_window = window;

    initWebGPU();
    initBuffers();
    initPipeline();
    initBufferData();

}

void 
WebGPURenderer::initWebGPU()
{
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    
    m_instance = wgpuCreateInstance(&desc);
    if (!m_instance)
    {
        ERR("Could not initilzie WebGPU Instance");
        return;
    }

    m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window->getGlfwWindow());

    m_adapter = requestAdapterSync(m_instance);
    if (!m_adapter) 
    {
        ERR("Could not initilzie WebGPU Adapter");
        return;
    }

    m_device = requestDeviceSync(m_adapter);

    if (!m_device) 
    {
        ERR("Could not initilzie WebGPU Device");
        return;
    }

    m_queue = wgpuDeviceGetQueue(m_device);

    resizeSurface(m_surface, m_adapter, m_device, m_window->getWidth(), m_window->getHeight());
}

void
WebGPURenderer::initBuffers() {
    // create input buffer
    m_input_buffer = std::make_unique<Buffer<float>>(m_device, 64L, (WGPUBufferUsage)(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst));

    // create output buffer
    m_output_buffer = std::make_unique<Buffer<float>>(m_device, 64L, (WGPUBufferUsage)(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc));

    // create copy buffer
    m_map_buffer = std::make_unique<Buffer<float>>(m_device, 64L, (WGPUBufferUsage)(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead));

    // create fullscreen quad buffer
    m_fullscreen_quad = std::make_unique<Buffer<float>>(m_device, 12L, (WGPUBufferUsage)(WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst));
}

void
WebGPURenderer::initTextures() {
}


void
WebGPURenderer::initPipeline() {
    // create shader modules
    m_pathtracing_shader = std::make_unique<Shader>(m_device, (char*)pathtracer_wgsl, "pathtracer");

    /* TODO: Avoid the need for two shader instances */
    m_postprocessing_shader_vert = std::make_unique<Shader>(m_device, (char*)postprocess_wgsl, "vs_main");
    m_postprocessing_shader_frag = std::make_unique<Shader>(m_device, (char*)postprocess_wgsl, "fs_main");

    // create pathtracing pipeline
    m_pathtracing_pipeline = std::make_unique<ComputePipeline>();
    m_pathtracing_pipeline->addBufferBinding(*m_input_buffer, 0, WGPUBufferBindingType_ReadOnlyStorage, WGPUShaderStage_Compute);
    m_pathtracing_pipeline->addBufferBinding(*m_output_buffer, 1, WGPUBufferBindingType_Storage, WGPUShaderStage_Compute);
    m_pathtracing_pipeline->addShader(*m_pathtracing_shader);
    m_pathtracing_pipeline->commit(m_device);

    // create postprocessing pipeline
    Texture draw_target(m_surface);
    m_postprocessing_pipeline = std::make_unique<RenderPipeline>();
    m_postprocessing_pipeline->addColorTarget(draw_target);
    m_postprocessing_pipeline->addVertexShader(*m_postprocessing_shader_vert);
    m_postprocessing_pipeline->addFragmentShader(*m_postprocessing_shader_frag);
    m_postprocessing_pipeline->addVertexAttribute(0, 0, WGPUVertexFormat_Float32x2, 0L);
    m_postprocessing_pipeline->addVertexBuffer(*m_fullscreen_quad, 2);
    m_postprocessing_pipeline->commit(m_device);

    /* we need this since we createa a texture from the surface without invalidating it */
    wgpuSurfacePresent(m_surface);
}

void
WebGPURenderer::initBufferData() {
    // fill buffers
    std::vector<float> input(m_input_buffer->getNElements());
    for (int i = 0; i < input.size(); ++i) {
        input[i] = 0.1f * (i+1);
    }

    m_input_buffer->setData(m_device, m_queue, input);

    std::vector<float> quad {
        -1, -1,
        -1,  1,
        1,  1,
        -1, -1,
        1,  1,
        1, -1
    };
    m_fullscreen_quad->setData(m_device, m_queue, quad);
}

void 
WebGPURenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) 
{
    auto t_start = std::chrono::high_resolution_clock::now();

    // Resize framebuffer if needed
    resizeSurface(m_surface, m_adapter, m_device, m_window->getWidth(), m_window->getHeight());

    // Run pathtracing pass
    renderpassPathtracer();

    // Run postprocessing pass
    renderpassPostprocess();

    // Present framebuffer
	wgpuSurfacePresent(m_surface);

    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;

}

void WebGPURenderer::renderpassPathtracer()
{ 
    // Create command encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoder_descriptor);

    // Create compute pass
    WGPUComputePassDescriptor computepass_descriptor = {};
    computepass_descriptor.timestampWrites = nullptr;
    computepass_descriptor.nextInChain = nullptr;
    WGPUComputePassEncoder computepass_encoder = wgpuCommandEncoderBeginComputePass(command_encoder, &computepass_descriptor);

    // Configure compute pass
    wgpuComputePassEncoderSetPipeline(computepass_encoder, m_pathtracing_pipeline->getComputePipeline());
    wgpuComputePassEncoderSetBindGroup(computepass_encoder, 0, m_pathtracing_pipeline->getBindGroup(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(computepass_encoder, 2, 1, 1);
    wgpuComputePassEncoderEnd(computepass_encoder);

    // Get ouputs
    wgpuCommandEncoderCopyBufferToBuffer(command_encoder, m_output_buffer->getBuffer(), 0, m_map_buffer->getBuffer(), 0, m_output_buffer->getSize());

    // Submit work
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, nullptr);
    wgpuQueueSubmit(m_queue, 1, &command_buffer);

    // Clean up
    wgpuComputePassEncoderRelease(computepass_encoder);
    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);
}

void
WebGPURenderer::renderpassPostprocess() {
    Texture draw_target(m_surface);
    if (!draw_target.getTextureView())
    {
        WARN("Could not get draw target from surface");
        return;
    }

    // Create command encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoder_descriptor);

    // The attachment part of the render pass descriptor describes the target texture of the pass
	WGPURenderPassColorAttachment color_attachment = {};
	color_attachment.view = draw_target.getTextureView();
	color_attachment.resolveTarget = nullptr;
	color_attachment.loadOp = WGPULoadOp_Clear;
	color_attachment.storeOp = WGPUStoreOp_Store;
	color_attachment.clearValue = WGPUColor{ 0.05, 0.05, 0.05, 1.0 };

    // Create render pass
    WGPURenderPassDescriptor renderpass_descriptor = {};
	renderpass_descriptor.colorAttachmentCount = 1;
	renderpass_descriptor.colorAttachments = &color_attachment;
	renderpass_descriptor.depthStencilAttachment = nullptr;
	renderpass_descriptor.timestampWrites = nullptr;

	WGPURenderPassEncoder renderpass_encoder = wgpuCommandEncoderBeginRenderPass(command_encoder, &renderpass_descriptor);

	// Select which render pipeline to use
	wgpuRenderPassEncoderSetPipeline(renderpass_encoder, m_postprocessing_pipeline->getRenderPipeline());
    wgpuRenderPassEncoderSetBindGroup(renderpass_encoder, 0, m_postprocessing_pipeline->getBindGroup(), 0, nullptr); 
	wgpuRenderPassEncoderSetVertexBuffer(renderpass_encoder, 0, m_fullscreen_quad->getBuffer(), 0, m_fullscreen_quad->getSize());

	wgpuRenderPassEncoderDraw(renderpass_encoder, 6, 1, 0, 0);

	wgpuRenderPassEncoderEnd(renderpass_encoder);
	wgpuRenderPassEncoderRelease(renderpass_encoder);

	// Encode and submit the render pass
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(command_encoder, nullptr);
	wgpuQueueSubmit(m_queue, 1, &command);
	wgpuCommandBufferRelease(command);
    wgpuCommandEncoderRelease(command_encoder);
}


/* From: https://eliemichel.github.io/LearnWebGPU/getting-started/adapter-and-device/the-adapter.html */
WGPUAdapter 
WebGPURenderer::requestAdapterSync(WGPUInstance instance) 
{
    WGPURequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.compatibleSurface = m_surface;

    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            ERR("Could not get WebGPU adapter");
            ERR(message);
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        &options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.adapter;
}


/* From: https://eliemichel.github.io/LearnWebGPU/getting-started/adapter-and-device/the-adapter.html */
WGPUDevice 
WebGPURenderer::requestDeviceSync(WGPUAdapter adapter) 
{
    WGPUSupportedLimits supported_limits;
    wgpuAdapterGetLimits(m_adapter, &supported_limits);
    WGPURequiredLimits required_limits = {};
    required_limits.limits.maxVertexBuffers = 1;
    required_limits.limits.maxVertexAttributes = 1;
    required_limits.limits.maxVertexBufferArrayStride = 8;
	required_limits.limits.maxBindGroups = 2;
	required_limits.limits.maxUniformBuffersPerShaderStage = 2;
	required_limits.limits.maxUniformBufferBindingSize = 64 * sizeof(float);
    required_limits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
	required_limits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
	required_limits.limits.maxBufferSize = 64 * 4;
	required_limits.limits.maxTextureDimension1D = 4096;
	required_limits.limits.maxTextureDimension2D = 4096;
	required_limits.limits.maxTextureDimension3D = 2048; 
	required_limits.limits.maxTextureArrayLayers = 1;
	required_limits.limits.maxSampledTexturesPerShaderStage = 3;
	required_limits.limits.maxSamplersPerShaderStage = 1;
	required_limits.limits.maxInterStageShaderComponents = 17;
	required_limits.limits.maxStorageBuffersPerShaderStage = 2;
	required_limits.limits.maxComputeWorkgroupSizeX = 32;
	required_limits.limits.maxComputeWorkgroupSizeY = 1;
	required_limits.limits.maxComputeWorkgroupSizeZ = 1;
	required_limits.limits.maxComputeInvocationsPerWorkgroup = 32;
	required_limits.limits.maxComputeWorkgroupsPerDimension = 2;
    required_limits.limits.maxStorageBufferBindingSize = 64 * 4;
    required_limits.limits.maxBindingsPerBindGroup = 2;

    WGPUDeviceDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;
    descriptor.label = "WebGPU Device"; 
    descriptor.requiredFeatureCount = 0;
    descriptor.requiredLimits = &required_limits;
    descriptor.defaultQueue.nextInChain = nullptr;
    descriptor.defaultQueue.label = "The default queue";
    descriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void*) {
        WARN("WebGPU device lost!");
        WARN(reason);
        if (message) WARN(message);
    };

    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(
        adapter,
        &descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.device;
}

void
WebGPURenderer::resizeSurface(WGPUSurface surface, WGPUAdapter adapter, WGPUDevice device, uint32_t width, uint32_t height)
{
    WGPUSurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    config.width = width;
    config.height = height;

    WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
    config.format = surfaceFormat;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device;

    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface, &config);
}

}