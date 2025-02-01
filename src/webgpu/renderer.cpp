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
    if (m_pathtracing_pipeline) {
        wgpuComputePipelineRelease(m_pathtracing_pipeline);
    }
    if (m_bindgroup_layout) {
        wgpuBindGroupLayoutRelease(m_bindgroup_layout);
    }
    if (m_bindgroup) {
        wgpuBindGroupRelease(m_bindgroup);
    }
    if (m_pipeline_layout) {
        wgpuPipelineLayoutRelease(m_pipeline_layout);
    }
}

void 
WebGPURenderer::init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window)
{
    m_scene = scene;
    m_window = window;

    initWebGPU();
    initBuffers();
    initBindgroupLayout();
    initBindgroup();
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
}

void
WebGPURenderer::initBuffers() {
    // create input buffer
    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.mappedAtCreation = false;
    buffer_descriptor.size = m_buffersize;
    buffer_descriptor.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
    m_input_buffer = wgpuDeviceCreateBuffer(m_device, &buffer_descriptor);

    // create output buffer
    buffer_descriptor.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc;
    m_output_buffer = wgpuDeviceCreateBuffer(m_device, &buffer_descriptor);

    // create copy buffer
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    m_map_buffer = wgpuDeviceCreateBuffer(m_device, &buffer_descriptor);
}

void
WebGPURenderer::initTextures() {
}

void
WebGPURenderer::initBindgroupLayout() {
    std::vector<WGPUBindGroupLayoutEntry> bindings(2);

    // input buffer
    bindings[0] = {};
    bindings[0].binding = 0;
    bindings[0].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    bindings[0].visibility = WGPUShaderStage_Compute;

    // output buffer
    bindings[1] = {};
    bindings[1].binding = 1;
    bindings[1].buffer.type = WGPUBufferBindingType_Storage;
    bindings[1].visibility = WGPUShaderStage_Compute;

    // assemble bindings
    WGPUBindGroupLayoutDescriptor bindgroup_layout_descriptor;
    bindgroup_layout_descriptor.entryCount = bindings.size();
    bindgroup_layout_descriptor.entries = bindings.data();
    m_bindgroup_layout = wgpuDeviceCreateBindGroupLayout(m_device, &bindgroup_layout_descriptor);
}

void 
WebGPURenderer::initBindgroup() {
    std::vector<WGPUBindGroupEntry> bindings(2);

    // input buffer
    bindings[0] = {};
    bindings[0].binding = 0;
    bindings[0].buffer = m_input_buffer;
    bindings[0].offset = 0;
    bindings[0].size = m_buffersize;

    bindings[1] = {};
    bindings[1].binding = 1;
    bindings[1].buffer = m_output_buffer;
    bindings[1].offset = 0;
    bindings[1].size = m_buffersize;

    WGPUBindGroupDescriptor bindgroup_descriptor = {};
    bindgroup_descriptor.layout = m_bindgroup_layout;
    bindgroup_descriptor.entryCount = bindings.size();
    bindgroup_descriptor.entries = bindings.data();
    m_bindgroup = wgpuDeviceCreateBindGroup(m_device, &bindgroup_descriptor);
}

void
WebGPURenderer::initPipeline() {
    // create shader module
    WGPUShaderModuleWGSLDescriptor shader_code_descriptor = {};
    shader_code_descriptor.chain.next = nullptr;
    shader_code_descriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shader_code_descriptor.code = (char*)pathtracer_wgsl;

    WGPUShaderModuleDescriptor shader_descriptor = {};
    shader_descriptor.hintCount = 0;
    shader_descriptor.hints = nullptr;
    shader_descriptor.nextInChain = &shader_code_descriptor.chain;

    WGPUShaderModule pathtracing_shader = wgpuDeviceCreateShaderModule(m_device, &shader_descriptor);

    // create pipeline layout
    WGPUPipelineLayoutDescriptor layout_descriptor = {};
    layout_descriptor.bindGroupLayoutCount = 1;
    layout_descriptor.bindGroupLayouts = &m_bindgroup_layout;
    m_pipeline_layout = wgpuDeviceCreatePipelineLayout(m_device, &layout_descriptor);

    // create pipline
    WGPUComputePipelineDescriptor pipeline_descriptor = {};
    pipeline_descriptor.compute.entryPoint = "pathtracer";
    pipeline_descriptor.compute.module = pathtracing_shader;
    pipeline_descriptor.layout = m_pipeline_layout;
    
    m_pathtracing_pipeline = wgpuDeviceCreateComputePipeline(m_device, &pipeline_descriptor);

    wgpuShaderModuleRelease(pathtracing_shader);
}

void
WebGPURenderer::initBufferData() {
    // fill buffers
    std::vector<float> input(m_buffersize / sizeof(float));
    for (int i = 0; i < input.size(); ++i) {
        input[i] = 0.1f * (i+1);
    }
    wgpuQueueWriteBuffer(m_queue, m_input_buffer, 0, input.data(), input.size() * sizeof(float));
}

void 
WebGPURenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) 
{
    auto t_start = std::chrono::high_resolution_clock::now();

    // Resize framebuffer if needed
    resizeSurface(m_surface, m_adapter, m_device, m_window->getWidth(), m_window->getHeight());

    // Run pathtracing pass
    renderpassPathtracer();

    // Read outputs
    renderpassReadOutputs();

    // Present framebuffer
	wgpuSurfacePresent(m_surface);

    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;

}

void WebGPURenderer::renderpassPathtracer()
{ 


    Texture draw_target(m_surface);
    if (!draw_target.getTextureView())
    {
        WARN("Could not get draw target from surface");
        return;
    }

    // Create command encoder for compute pass
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoder_descriptor);

    // Create compute pass
    WGPUComputePassDescriptor computepass_descriptor = {};
    computepass_descriptor.timestampWrites = nullptr;
    computepass_descriptor.nextInChain = nullptr;
    WGPUComputePassEncoder computepass_encoder = wgpuCommandEncoderBeginComputePass(command_encoder, &computepass_descriptor);

    // Configure compute pass
    wgpuComputePassEncoderSetPipeline(computepass_encoder, m_pathtracing_pipeline);
    wgpuComputePassEncoderSetBindGroup(computepass_encoder, 0, m_bindgroup, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(computepass_encoder, 2, 1, 1);
    wgpuComputePassEncoderEnd(computepass_encoder);

    // Get ouputs
    wgpuCommandEncoderCopyBufferToBuffer(command_encoder, m_output_buffer, 0, m_map_buffer, 0, m_buffersize);

    // Submit work
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, nullptr);
    wgpuQueueSubmit(m_queue, 1, &command_buffer);

    // Clean up
    wgpuComputePassEncoderRelease(computepass_encoder);
    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);
}

void
WebGPURenderer::renderpassReadOutputs() {
    mappingdone = false;
    wgpuBufferMapAsync(m_map_buffer, WGPUMapMode_Read, 0, m_buffersize, 
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            WebGPURenderer* renderer = (WebGPURenderer*) userdata;
            if (status == WGPUBufferMapAsyncStatus_Success) {
                const float* output = (const float*)wgpuBufferGetConstMappedRange(renderer->m_map_buffer, 0, renderer->m_buffersize);

                for (size_t i = 0; i < renderer->m_buffersize/sizeof(float); ++i) {
                    LOG(output[i]);
                }
            }
            wgpuBufferUnmap(renderer->m_map_buffer);
            renderer->mappingdone = true;
        }, this);

    while (!mappingdone) {

    }
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
	required_limits.limits.maxBindGroups = 2;
	required_limits.limits.maxUniformBuffersPerShaderStage = 2;
	required_limits.limits.maxUniformBufferBindingSize = 64 * sizeof(float);
    required_limits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
	required_limits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
	required_limits.limits.maxBufferSize = m_buffersize;
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
    required_limits.limits.maxStorageBufferBindingSize = m_buffersize;
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